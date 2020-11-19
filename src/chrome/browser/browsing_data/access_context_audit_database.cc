// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/browsing_data/access_context_audit_database.h"

#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/metrics/histogram_functions.h"
#include "base/rand_util.h"
#include "net/cookies/cookie_util.h"
#include "sql/database.h"
#include "sql/error_delegate_util.h"
#include "sql/meta_table.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace {

const base::FilePath::CharType kDatabaseName[] =
    FILE_PATH_LITERAL("AccessContextAudit");
const char kCookieTableName[] = "cookies";
const char kStorageAPITableName[] = "originStorageAPIs";
static const int kVersionNumber = 1;

// Callback that is fired upon an SQLite error, razes the database if the error
// is considered catastrphoic.
void DatabaseErrorCallback(sql::Database* db,
                           const base::FilePath& db_path,
                           int extended_error,
                           sql::Statement* stmt) {
  if (sql::IsErrorCatastrophic(extended_error)) {
    // Prevent reentrant calls.
    db->reset_error_callback();

    // After this call, the |db| handle is poisoned so that future calls will
    // return errors until the handle is re-opened.
    db->RazeAndClose();

    // The DLOG(WARNING) below is intended to draw immediate attention to errors
    // in newly-written code.  Database corruption is generally a result of OS
    // or hardware issues, not coding errors at the client level, so displaying
    // the error would probably lead to confusion.  The ignored call signals the
    // test-expectation framework that the error was handled.
    ignore_result(sql::Database::IsExpectedSqliteError(extended_error));
    return;
  }

  // The default handling is to assert on debug and to ignore on release.
  if (!sql::Database::IsExpectedSqliteError(extended_error))
    DLOG(FATAL) << db->GetErrorMessage();
}

// Returns true if a cookie table already exists in |db|, but is missing the
// is_persistent field.
bool CookieTableMissingIsPersistent(sql::Database* db) {
  std::string select = "SELECT sql FROM sqlite_master WHERE name = '";
  select.append(kCookieTableName);
  select.append("' AND type = 'table'");
  sql::Statement statement(db->GetUniqueStatement(select.c_str()));

  // Unable to step implies cookies table does not exist.
  if (!statement.Step())
    return false;

  std::string cookies_schema = statement.ColumnString(0);
  return cookies_schema.find("is_persistent") == std::string::npos;
}

// Removes all cookie records in |db| with is_persistent = false.
bool DeleteNonPersistentCookies(sql::Database* db) {
  std::string remove = "DELETE FROM ";
  remove.append(kCookieTableName);
  remove.append(" WHERE is_persistent != 1");
  return db->Execute(remove.c_str());
}

bool IsContentSettingSessionOnly(
    const GURL& url,
    const ContentSettingsForOneType& content_settings) {
  // ContentSettingsForOneType are in order of decreasing specificity, such
  // that the first matching entry defines the effective content setting.
  for (const auto& setting : content_settings) {
    // A match is performed against both primary and secondary patterns. This
    // aligns with the behavior in CookieSettingsBase::ShouldDeleteCookieOnExit,
    // which is used by the cookie store.
    if (setting.primary_pattern.Matches(url) &&
        setting.secondary_pattern.Matches(url)) {
      return setting.GetContentSetting() ==
             ContentSetting::CONTENT_SETTING_SESSION_ONLY;
    }
  }
  NOTREACHED();
  return false;
}

}  // namespace

AccessContextAuditDatabase::AccessRecord::AccessRecord(
    const url::Origin& top_frame_origin,
    const std::string& name,
    const std::string& domain,
    const std::string& path,
    const base::Time& last_access_time,
    bool is_persistent)
    : top_frame_origin(top_frame_origin),
      type(StorageAPIType::kCookie),
      name(name),
      domain(domain),
      path(path),
      last_access_time(last_access_time),
      is_persistent(is_persistent) {}

AccessContextAuditDatabase::AccessRecord::AccessRecord(
    const url::Origin& top_frame_origin,
    const StorageAPIType& type,
    const url::Origin& origin,
    const base::Time& last_access_time)
    : top_frame_origin(top_frame_origin),
      type(type),
      origin(origin),
      last_access_time(last_access_time) {
  DCHECK(type != StorageAPIType::kCookie);
}

AccessContextAuditDatabase::AccessRecord::~AccessRecord() = default;

AccessContextAuditDatabase::AccessRecord::AccessRecord(
    const AccessRecord& other) = default;

AccessContextAuditDatabase::AccessRecord&
AccessContextAuditDatabase::AccessRecord::operator=(const AccessRecord& other) =
    default;

AccessContextAuditDatabase::AccessContextAuditDatabase(
    const base::FilePath& path_to_database_dir)
    : db_file_path_(path_to_database_dir.Append(kDatabaseName)) {}

void AccessContextAuditDatabase::Init(bool restore_non_persistent_cookies) {
  db_.set_histogram_tag("Access Context Audit");

  db_.set_error_callback(
      base::BindRepeating(&DatabaseErrorCallback, &db_, db_file_path_));

  // Cache values generated assuming ~5000 individual pieces of client storage
  // API data, each accessed in an average of 3 different contexts (complete
  // speculation, most will be 1, some will be >50), with an average of
  // 40bytes per audit entry.
  // TODO(crbug.com/1083384): Revist these numbers.
  db_.set_page_size(4096);
  db_.set_cache_size(128);

  db_.set_exclusive_locking();

  if (!db_.Open(db_file_path_))
    return;

  // Scope database initialisation in a transaction.
  sql::Transaction transaction(&db_);
  if (!transaction.Begin())
    return;

  if (!meta_table_.Init(&db_, kVersionNumber, kVersionNumber))
    return;

  if (meta_table_.GetCompatibleVersionNumber() > kVersionNumber) {
    LOG(ERROR) << "Access Context Audit database is too new, kVersionNumber"
               << kVersionNumber << ", GetCompatibleVersionNumber="
               << meta_table_.GetCompatibleVersionNumber();
    // No error will have been caught by the SQLite error handler, manually
    // shut the the database.
    transaction.Rollback();
    db_.Close();
    return;
  }

  if (!InitializeSchema())
    return;

  if (!restore_non_persistent_cookies)
    DeleteNonPersistentCookies(&db_);

  transaction.Commit();

  // Computing metrics is costly, only perform it in 1% of startups.
  if (base::RandInt(1, 100) == 50)
    ComputeDatabaseMetrics();
}

bool AccessContextAuditDatabase::InitializeSchema() {
  if (CookieTableMissingIsPersistent(&db_)) {
    // Simply remove the table in this case. Due to a flag misconfiguration this
    // version of the table was pushed to all canary users for a short period.
    // TODO(crbug.com/1102006): Remove this code before M86 branch point.
    std::string drop_table = "DROP TABLE ";
    drop_table.append(kCookieTableName);
    if (!db_.Execute(drop_table.c_str()))
      return false;
  }

  std::string create_table;
  create_table.append("CREATE TABLE IF NOT EXISTS ");
  create_table.append(kCookieTableName);
  create_table.append(
      "(top_frame_origin TEXT NOT NULL,"
      "name TEXT NOT NULL,"
      "domain TEXT NOT NULL,"
      "path TEXT NOT NULL,"
      "access_utc INTEGER NOT NULL,"
      "is_persistent INTEGER NOT NULL,"
      "PRIMARY KEY (top_frame_origin, name, domain, path))");

  if (!db_.Execute(create_table.c_str()))
    return false;

  create_table.clear();
  create_table.append("CREATE TABLE IF NOT EXISTS ");
  create_table.append(kStorageAPITableName);
  create_table.append(
      "(top_frame_origin TEXT NOT NULL,"
      "type INTEGER NOT NULL,"
      "origin TEXT NOT NULL,"
      "access_utc INTEGER NOT NULL,"
      "PRIMARY KEY (top_frame_origin, origin, type))");

  return db_.Execute(create_table.c_str());
}

void AccessContextAuditDatabase::ComputeDatabaseMetrics() {
  // Calculate database file size in KB.
  int64_t file_size = 0;
  if (!base::GetFileSize(db_file_path_, &file_size))
    return;
  base::UmaHistogramMemoryKB("Privacy.AccessContextAudit.DatabaseSize",
                             static_cast<int>(file_size / 1024));

  // Count the total number of records stored.
  std::string record_count =
      "SELECT (SELECT COUNT(*) FROM cookies) + "
      "(SELECT COUNT(*) FROM originStorageAPIs)";
  sql::Statement record_count_statement(
      db_.GetCachedStatement(SQL_FROM_HERE, record_count.c_str()));
  base::UmaHistogramCounts100000(
      "Privacy.AccessContextAudit.RecordCount",
      record_count_statement.Step() ? record_count_statement.ColumnInt(0) : 0);

  // Count the unique number of unique top frame origins.
  std::string top_frame_origin_count =
      "SELECT COUNT(*) FROM (SELECT DISTINCT top_frame_origin FROM cookies"
      " UNION SELECT DISTINCT top_frame_origin FROM originStorageAPIs)";
  sql::Statement top_frame_origin_count_statement(
      db_.GetCachedStatement(SQL_FROM_HERE, top_frame_origin_count.c_str()));
  base::UmaHistogramCounts10000(
      "Privacy.AccessContextAudit.TopFrameOriginCount",
      top_frame_origin_count_statement.Step()
          ? top_frame_origin_count_statement.ColumnInt(0)
          : 0);

  // Count the number of unique origins using origin keyed storage APIs.
  std::string storage_origin_count =
      "SELECT COUNT(DISTINCT origin) FROM originStorageAPIs";
  sql::Statement storage_origin_count_statement(
      db_.GetCachedStatement(SQL_FROM_HERE, storage_origin_count.c_str()));
  base::UmaHistogramCounts10000(
      "Privacy.AccessContextAudit.StorageOriginCount",
      storage_origin_count_statement.Step()
          ? storage_origin_count_statement.ColumnInt(0)
          : 0);

  // Count the number of unique cookie domains.
  std::string cookie_domain_count =
      "SELECT COUNT(DISTINCT domain) FROM cookies";
  sql::Statement cookie_domain_count_statement(
      db_.GetCachedStatement(SQL_FROM_HERE, cookie_domain_count.c_str()));
  base::UmaHistogramCounts10000("Privacy.AccessContextAudit.CookieDomainCount",
                                cookie_domain_count_statement.Step()
                                    ? cookie_domain_count_statement.ColumnInt(0)
                                    : 0);
}

void AccessContextAuditDatabase::AddRecords(
    const std::vector<AccessContextAuditDatabase::AccessRecord>& records) {
  sql::Transaction transaction(&db_);
  if (!transaction.Begin())
    return;

  // Create both insert statements ahead of iterating over records. These are
  // highly likely to both be used, and should be in the statement cache.
  std::string insert;
  insert.append("INSERT OR REPLACE INTO ");
  insert.append(kCookieTableName);
  insert.append(
      "(top_frame_origin, name, domain, path, access_utc, is_persistent) "
      "VALUES (?, ?, ?, ?, ?, ?)");
  sql::Statement insert_cookie(
      db_.GetCachedStatement(SQL_FROM_HERE, insert.c_str()));

  insert.clear();
  insert.append("INSERT OR REPLACE INTO ");
  insert.append(kStorageAPITableName);
  insert.append(
      "(top_frame_origin, type, origin, access_utc) "
      "VALUES (?, ?, ?, ?)");
  sql::Statement insert_storage_api(
      db_.GetCachedStatement(SQL_FROM_HERE, insert.c_str()));

  for (const auto& record : records) {
    if (record.type == StorageAPIType::kCookie) {
      insert_cookie.BindString(0, record.top_frame_origin.Serialize());
      insert_cookie.BindString(1, record.name);
      insert_cookie.BindString(2, record.domain);
      insert_cookie.BindString(3, record.path);
      insert_cookie.BindInt64(
          4,
          record.last_access_time.ToDeltaSinceWindowsEpoch().InMicroseconds());
      insert_cookie.BindBool(5, record.is_persistent);

      if (!insert_cookie.Run())
        return;

      insert_cookie.Reset(true);
    } else {
      insert_storage_api.BindString(0, record.top_frame_origin.Serialize());
      insert_storage_api.BindInt(1, static_cast<int>(record.type));
      insert_storage_api.BindString(2, record.origin.Serialize());
      insert_storage_api.BindInt64(
          3,
          record.last_access_time.ToDeltaSinceWindowsEpoch().InMicroseconds());

      if (!insert_storage_api.Run())
        return;

      insert_storage_api.Reset(true);
    }
  }

  transaction.Commit();
}

void AccessContextAuditDatabase::RemoveRecord(const AccessRecord& record) {
  sql::Statement remove_statement;

  std::string remove;
  remove.append("DELETE FROM ");
  if (record.type == StorageAPIType::kCookie) {
    remove.append(kCookieTableName);
    remove.append(
        " WHERE top_frame_origin = ? AND name = ? AND domain = ? AND path = ?");
    remove_statement.Assign(
        db_.GetCachedStatement(SQL_FROM_HERE, remove.c_str()));
    remove_statement.BindString(0, record.top_frame_origin.Serialize());
    remove_statement.BindString(1, record.name);
    remove_statement.BindString(2, record.domain);
    remove_statement.BindString(3, record.path);
  } else {
    remove.append(kStorageAPITableName);
    remove.append(" WHERE top_frame_origin = ? AND type = ? AND origin = ?");
    remove_statement.Assign(
        db_.GetCachedStatement(SQL_FROM_HERE, remove.c_str()));
    remove_statement.BindString(0, record.top_frame_origin.Serialize());
    remove_statement.BindInt(1, static_cast<int>(record.type));
    remove_statement.BindString(2, record.origin.Serialize());
  }
  remove_statement.Run();
}

void AccessContextAuditDatabase::RemoveAllRecords() {
  // Perform deletions with the WHERE clause omitted to trigger the SQLite table
  // truncation optimisation.
  sql::Transaction transaction(&db_);
  if (!transaction.Begin())
    return;

  std::string delete_cookies_table = "DELETE FROM ";
  delete_cookies_table.append(kCookieTableName);
  if (!db_.Execute(delete_cookies_table.c_str()))
    return;

  std::string delete_storage_api_table = "DELETE FROM ";
  delete_storage_api_table.append(kStorageAPITableName);
  if (!db_.Execute(delete_storage_api_table.c_str()))
    return;

  transaction.Commit();
}

void AccessContextAuditDatabase::RemoveAllRecordsForTimeRange(base::Time begin,
                                                              base::Time end) {
  sql::Transaction transaction(&db_);
  if (!transaction.Begin())
    return;

  std::string remove = "DELETE FROM ";
  remove.append(kCookieTableName);
  remove.append(" WHERE access_utc BETWEEN ? AND ?");
  sql::Statement remove_cookies(
      db_.GetCachedStatement(SQL_FROM_HERE, remove.c_str()));
  remove_cookies.BindInt64(0,
                           begin.ToDeltaSinceWindowsEpoch().InMicroseconds());
  remove_cookies.BindInt64(1, end.ToDeltaSinceWindowsEpoch().InMicroseconds());
  if (!remove_cookies.Run())
    return;

  remove = "DELETE FROM ";
  remove.append(kStorageAPITableName);
  remove.append(" WHERE access_utc BETWEEN ? AND ?");
  sql::Statement remove_storage_apis(
      db_.GetCachedStatement(SQL_FROM_HERE, remove.c_str()));
  remove_storage_apis.BindInt64(
      0, begin.ToDeltaSinceWindowsEpoch().InMicroseconds());
  remove_storage_apis.BindInt64(
      1, end.ToDeltaSinceWindowsEpoch().InMicroseconds());
  if (!remove_storage_apis.Run())
    return;

  transaction.Commit();
}

void AccessContextAuditDatabase::RemoveSessionOnlyRecords(
    const ContentSettingsForOneType& content_settings) {
  // ContentSettingsForOneType is a list of settings in decreasing specificity
  // for origins, ending with a setting that matches all and is the default.
  DCHECK(content_settings.size());
  if (content_settings.size() == 1) {
    DCHECK_EQ(content_settings[0].primary_pattern,
              ContentSettingsPattern::Wildcard());
    DCHECK_EQ(content_settings[0].secondary_pattern,
              ContentSettingsPattern::Wildcard());
    if (content_settings[0].GetContentSetting() ==
        ContentSetting::CONTENT_SETTING_SESSION_ONLY) {
      RemoveAllRecords();
    }
    // As only the default content setting is set, there is no need to inspect
    // any individual records.
    return;
  }

  sql::Transaction transaction(&db_);
  if (!transaction.Begin())
    return;

  // Extract the set of all domains from the cookies table, determine the
  // effective content setting, and store for removal if appropriate.
  std::string select = "SELECT DISTINCT domain FROM ";
  select.append(kCookieTableName);
  sql::Statement select_cookie_domains(
      db_.GetCachedStatement(SQL_FROM_HERE, select.c_str()));

  std::vector<std::string> cookie_domains_for_removal;
  while (select_cookie_domains.Step()) {
    auto domain = select_cookie_domains.ColumnString(0);
    GURL url = net::cookie_util::CookieOriginToURL(domain,
                                                   /* is_https */ false);
    GURL secure_url = net::cookie_util::CookieOriginToURL(domain,
                                                          /* is_https */ true);
    if (IsContentSettingSessionOnly(url, content_settings) ||
        IsContentSettingSessionOnly(secure_url, content_settings)) {
      cookie_domains_for_removal.emplace_back(std::move(domain));
    }
  }

  // Repeat the above, but for the origin keyed storage API table.
  select = "SELECT DISTINCT origin FROM ";
  select.append(kStorageAPITableName);
  sql::Statement select_storage_origins(
      db_.GetCachedStatement(SQL_FROM_HERE, select.c_str()));

  std::vector<std::string> storage_origins_for_removal;
  while (select_storage_origins.Step()) {
    auto origin = select_storage_origins.ColumnString(0);
    if (IsContentSettingSessionOnly(GURL(origin), content_settings))
      storage_origins_for_removal.emplace_back(origin);
  }

  // Remove entries belonging to cookie domains and origins identified as having
  // a SESSION_ONLY content setting.
  std::string remove = "DELETE FROM ";
  remove.append(kCookieTableName);
  remove.append(" WHERE domain = ?");
  sql::Statement remove_cookies(
      db_.GetCachedStatement(SQL_FROM_HERE, remove.c_str()));

  for (const auto& domain : cookie_domains_for_removal) {
    remove_cookies.BindString(0, domain);
    if (!remove_cookies.Run())
      return;
    remove_cookies.Reset(true);
  }

  remove = "DELETE FROM ";
  remove.append(kStorageAPITableName);
  remove.append(" WHERE origin = ?");
  sql::Statement remove_storage_apis(
      db_.GetCachedStatement(SQL_FROM_HERE, remove.c_str()));

  for (const auto& origin : storage_origins_for_removal) {
    remove_storage_apis.BindString(0, origin);
    if (!remove_storage_apis.Run())
      return;
    remove_storage_apis.Reset(true);
  }

  transaction.Commit();
}

void AccessContextAuditDatabase::RemoveAllRecordsForCookie(
    const std::string& name,
    const std::string& domain,
    const std::string& path) {
  std::string remove;
  remove.append("DELETE FROM ");
  remove.append(kCookieTableName);
  remove.append(" WHERE name = ? AND domain = ? AND path = ?");
  sql::Statement remove_statement(
      db_.GetCachedStatement(SQL_FROM_HERE, remove.c_str()));
  remove_statement.BindString(0, name);
  remove_statement.BindString(1, domain);
  remove_statement.BindString(2, path);
  remove_statement.Run();
}

void AccessContextAuditDatabase::RemoveAllRecordsForOriginKeyedStorage(
    const url::Origin& origin,
    StorageAPIType type) {
  std::string remove;
  remove.append("DELETE FROM ");
  remove.append(kStorageAPITableName);
  remove.append(" WHERE origin = ? AND type = ?");
  sql::Statement remove_statement(
      db_.GetCachedStatement(SQL_FROM_HERE, remove.c_str()));
  remove_statement.BindString(0, origin.Serialize());
  remove_statement.BindInt(1, static_cast<int>(type));
  remove_statement.Run();
}

void AccessContextAuditDatabase::RemoveAllRecordsForTopFrameOrigins(
    const std::vector<url::Origin>& origins) {
  sql::Transaction transaction(&db_);
  if (!transaction.Begin())
    return;

  // Remove all records with a top frame origin present in |origins| from both
  // the cookies and storage API tables.
  std::string remove = "DELETE FROM ";
  remove.append(kCookieTableName);
  remove.append(" WHERE top_frame_origin = ?");
  sql::Statement remove_cookies(
      db_.GetCachedStatement(SQL_FROM_HERE, remove.c_str()));

  remove = "DELETE FROM ";
  remove.append(kStorageAPITableName);
  remove.append(" WHERE top_frame_origin = ?");
  sql::Statement remove_storage_apis(
      db_.GetCachedStatement(SQL_FROM_HERE, remove.c_str()));

  for (const auto& origin : origins) {
    remove_storage_apis.BindString(0, origin.Serialize());
    if (!remove_storage_apis.Run())
      return;
    remove_storage_apis.Reset(true);

    remove_cookies.BindString(0, origin.Serialize());
    if (!remove_cookies.Run())
      return;
    remove_cookies.Reset(true);
  }

  transaction.Commit();
}

std::vector<AccessContextAuditDatabase::AccessRecord>
AccessContextAuditDatabase::GetAllRecords() {
  std::vector<AccessContextAuditDatabase::AccessRecord> records;

  std::string select;
  select.append(
      "SELECT top_frame_origin, name, domain, path, access_utc, is_persistent "
      "FROM ");
  select.append(kCookieTableName);
  sql::Statement select_cookies(
      db_.GetCachedStatement(SQL_FROM_HERE, select.c_str()));

  while (select_cookies.Step()) {
    records.emplace_back(
        url::Origin::Create(GURL(select_cookies.ColumnString(0))),
        select_cookies.ColumnString(1), select_cookies.ColumnString(2),
        select_cookies.ColumnString(3),
        base::Time::FromDeltaSinceWindowsEpoch(
            base::TimeDelta::FromMicroseconds(select_cookies.ColumnInt64(4))),
        select_cookies.ColumnBool(5));
  }

  select.clear();
  select.append("SELECT top_frame_origin, type, origin, access_utc FROM ");
  select.append(kStorageAPITableName);
  sql::Statement select_storage_api(
      db_.GetCachedStatement(SQL_FROM_HERE, select.c_str()));

  while (select_storage_api.Step()) {
    records.emplace_back(
        url::Origin::Create(GURL(select_storage_api.ColumnString(0))),
        static_cast<StorageAPIType>(select_storage_api.ColumnInt(1)),
        url::Origin::Create(GURL(select_storage_api.ColumnString(2))),
        base::Time::FromDeltaSinceWindowsEpoch(
            base::TimeDelta::FromMicroseconds(
                select_storage_api.ColumnInt64(3))));
  }

  return records;
}

void AccessContextAuditDatabase::RemoveStorageApiRecords(
    const std::set<StorageAPIType>& storage_api_types,
    base::RepeatingCallback<bool(const url::Origin&)> origin_matcher,
    base::Time begin,
    base::Time end) {
  sql::Transaction transaction(&db_);
  if (!transaction.Begin())
    return;

  // As only the time ranges can be simply expressed as part of an SQL query,
  // removing records that match the provided parameters requires first
  // retrieving all records within the specified time range from the database.
  // The number of records retrieved is sub-optimal by at most a factor of
  // StorageAPIType::kMaxType, so we're not missing sub-linear optimization
  // opportunity here.
  std::string select = "SELECT origin, type FROM ";
  select.append(kStorageAPITableName);
  select.append(" WHERE access_utc BETWEEN ? AND ?");
  sql::Statement select_storage_api(
      db_.GetCachedStatement(SQL_FROM_HERE, select.c_str()));
  select_storage_api.BindInt64(
      0, begin.ToDeltaSinceWindowsEpoch().InMicroseconds());
  select_storage_api.BindInt64(1,
                               end.ToDeltaSinceWindowsEpoch().InMicroseconds());

  // Filter the returned records based on the provided parameters, maintaining
  // a list of origin and storage type pairs for removal from the database.
  std::vector<std::pair<url::Origin, StorageAPIType>>
      origin_type_pairs_for_removal;
  while (select_storage_api.Step()) {
    auto origin = url::Origin::Create(GURL(select_storage_api.ColumnString(0)));
    auto type = static_cast<StorageAPIType>(select_storage_api.ColumnInt(1));
    if (storage_api_types.count(type) &&
        (!origin_matcher || origin_matcher.Run(origin))) {
      origin_type_pairs_for_removal.emplace_back(origin, type);
    }
  }

  std::string remove = "DELETE FROM ";
  remove.append(kStorageAPITableName);
  remove.append(" WHERE origin = ? AND type = ?");
  sql::Statement remove_statement(
      db_.GetCachedStatement(SQL_FROM_HERE, remove.c_str()));

  for (const auto& origin_type : origin_type_pairs_for_removal) {
    remove_statement.BindString(0, origin_type.first.Serialize());
    remove_statement.BindInt(1, static_cast<int>(origin_type.second));
    if (!remove_statement.Run())
      return;
    remove_statement.Reset(true);
  }

  transaction.Commit();
}