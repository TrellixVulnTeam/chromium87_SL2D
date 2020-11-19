// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/driver/sync_service_crypto.h"

#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/feature_list.h"
#include "base/metrics/histogram_macros.h"
#include "base/no_destructor.h"
#include "base/sequenced_task_runner.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "components/sync/base/passphrase_enums.h"
#include "components/sync/base/sync_prefs.h"
#include "components/sync/driver/sync_driver_switches.h"
#include "components/sync/driver/sync_service.h"
#include "components/sync/engine/sync_engine_switches.h"
#include "components/sync/engine/sync_string_conversions.h"
#include "components/sync/nigori/nigori.h"

namespace syncer {

namespace {

// Used for UMA.
enum class TrustedVaultFetchKeysAttemptForUMA {
  kFirstAttempt,
  kSecondAttempt,
  kMaxValue = kSecondAttempt
};

// Used for the case where a null client is passed to SyncServiceCrypto.
class EmptyTrustedVaultClient : public TrustedVaultClient {
 public:
  EmptyTrustedVaultClient() = default;
  ~EmptyTrustedVaultClient() override = default;

  // TrustedVaultClient implementation.
  std::unique_ptr<Subscription> AddKeysChangedObserver(
      const base::RepeatingClosure& cb) override {
    return nullptr;
  }

  void FetchKeys(
      const CoreAccountInfo& account_info,
      base::OnceCallback<void(const std::vector<std::vector<uint8_t>>&)> cb)
      override {
    std::move(cb).Run({});
  }

  void StoreKeys(const std::string& gaia_id,
                 const std::vector<std::vector<uint8_t>>& keys,
                 int last_key_version) override {
    // Never invoked by SyncServiceCrypto.
    NOTREACHED();
  }

  void RemoveAllStoredKeys() override {
    // Never invoked by SyncServiceCrypto.
    NOTREACHED();
  }

  void MarkKeysAsStale(const CoreAccountInfo& account_info,
                       base::OnceCallback<void(bool)> cb) override {
    std::move(cb).Run(false);
  }

  void GetIsRecoverabilityDegraded(const CoreAccountInfo& account_info,
                                   base::OnceCallback<void(bool)> cb) override {
    std::move(cb).Run(false);
  }
};

// A SyncEncryptionHandler::Observer implementation that simply posts all calls
// to another task runner.
class SyncEncryptionObserverProxy : public SyncEncryptionHandler::Observer {
 public:
  SyncEncryptionObserverProxy(
      base::WeakPtr<SyncEncryptionHandler::Observer> observer,
      scoped_refptr<base::SequencedTaskRunner> task_runner)
      : observer_(observer), task_runner_(std::move(task_runner)) {}

  void OnPassphraseRequired(
      PassphraseRequiredReason reason,
      const KeyDerivationParams& key_derivation_params,
      const sync_pb::EncryptedData& pending_keys) override {
    task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&SyncEncryptionHandler::Observer::OnPassphraseRequired,
                       observer_, reason, key_derivation_params, pending_keys));
  }

  void OnPassphraseAccepted() override {
    task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&SyncEncryptionHandler::Observer::OnPassphraseAccepted,
                       observer_));
  }

  void OnTrustedVaultKeyRequired() override {
    task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(
            &SyncEncryptionHandler::Observer::OnTrustedVaultKeyRequired,
            observer_));
  }

  void OnTrustedVaultKeyAccepted() override {
    task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(
            &SyncEncryptionHandler::Observer::OnTrustedVaultKeyAccepted,
            observer_));
  }

  void OnBootstrapTokenUpdated(const std::string& bootstrap_token,
                               BootstrapTokenType type) override {
    task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(
            &SyncEncryptionHandler::Observer::OnBootstrapTokenUpdated,
            observer_, bootstrap_token, type));
  }

  void OnEncryptedTypesChanged(ModelTypeSet encrypted_types,
                               bool encrypt_everything) override {
    task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(
            &SyncEncryptionHandler::Observer::OnEncryptedTypesChanged,
            observer_, encrypted_types, encrypt_everything));
  }

  void OnEncryptionComplete() override {
    task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&SyncEncryptionHandler::Observer::OnEncryptionComplete,
                       observer_));
  }

  void OnCryptographerStateChanged(Cryptographer* cryptographer,
                                   bool has_pending_keys) override {
    task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(
            &SyncEncryptionHandler::Observer::OnCryptographerStateChanged,
            observer_, /*cryptographer=*/nullptr, has_pending_keys));
  }

  void OnPassphraseTypeChanged(PassphraseType type,
                               base::Time passphrase_time) override {
    task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(
            &SyncEncryptionHandler::Observer::OnPassphraseTypeChanged,
            observer_, type, passphrase_time));
  }

 private:
  base::WeakPtr<SyncEncryptionHandler::Observer> observer_;
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
};

TrustedVaultClient* ResoveNullClient(TrustedVaultClient* client) {
  if (client) {
    return client;
  }

  static base::NoDestructor<EmptyTrustedVaultClient> empty_client;
  return empty_client.get();
}

// Checks if |passphrase| can be used to decrypt the given pending keys. Returns
// true if decryption was successful. Returns false otherwise. Must be called
// with non-empty pending keys cache.
bool CheckPassphraseAgainstPendingKeys(
    const sync_pb::EncryptedData& pending_keys,
    const KeyDerivationParams& key_derivation_params,
    const std::string& passphrase) {
  DCHECK(pending_keys.has_blob());
  DCHECK(!passphrase.empty());
  if (key_derivation_params.method() == KeyDerivationMethod::UNSUPPORTED) {
    DLOG(ERROR) << "Cannot derive keys using an unsupported key derivation "
                   "method. Rejecting passphrase.";
    return false;
  }

  std::unique_ptr<Nigori> nigori =
      Nigori::CreateByDerivation(key_derivation_params, passphrase);
  DCHECK(nigori);
  std::string plaintext;
  bool decrypt_result = nigori->Decrypt(pending_keys.blob(), &plaintext);
  DVLOG_IF(1, !decrypt_result) << "Passphrase failed to decrypt pending keys.";
  return decrypt_result;
}

}  // namespace

SyncServiceCrypto::State::State()
    : passphrase_key_derivation_params(KeyDerivationParams::CreateForPbkdf2()) {
}

SyncServiceCrypto::State::~State() = default;

SyncServiceCrypto::SyncServiceCrypto(
    const base::RepeatingClosure& notify_observers,
    const base::RepeatingClosure& notify_required_user_action_changed,
    const base::RepeatingCallback<void(ConfigureReason)>& reconfigure,
    CryptoSyncPrefs* sync_prefs,
    TrustedVaultClient* trusted_vault_client)
    : notify_observers_(notify_observers),
      notify_required_user_action_changed_(notify_required_user_action_changed),
      reconfigure_(reconfigure),
      sync_prefs_(sync_prefs),
      trusted_vault_client_(ResoveNullClient(trusted_vault_client)) {
  DCHECK(notify_observers_);
  DCHECK(reconfigure_);
  DCHECK(sync_prefs_);
  DCHECK(trusted_vault_client_);

  trusted_vault_client_subscription_ =
      trusted_vault_client_->AddKeysChangedObserver(base::BindRepeating(
          &SyncServiceCrypto::OnTrustedVaultClientKeysChanged,
          weak_factory_.GetWeakPtr()));
}

SyncServiceCrypto::~SyncServiceCrypto() = default;

void SyncServiceCrypto::Reset() {
  state_ = State();
}

base::Time SyncServiceCrypto::GetExplicitPassphraseTime() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return state_.cached_explicit_passphrase_time;
}

bool SyncServiceCrypto::IsPassphraseRequired() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  switch (state_.required_user_action) {
    case RequiredUserAction::kUnknownDuringInitialization:
    case RequiredUserAction::kNone:
    case RequiredUserAction::kFetchingTrustedVaultKeys:
    case RequiredUserAction::kTrustedVaultKeyRequired:
    case RequiredUserAction::kTrustedVaultKeyRequiredButFetching:
    case RequiredUserAction::kTrustedVaultRecoverabilityDegraded:
      return false;
    case RequiredUserAction::kPassphraseRequiredForDecryption:
    case RequiredUserAction::kPassphraseRequiredForEncryption:
      return true;
  }

  NOTREACHED();
  return false;
}

bool SyncServiceCrypto::IsUsingSecondaryPassphrase() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return IsExplicitPassphrase(state_.cached_passphrase_type);
}

bool SyncServiceCrypto::IsTrustedVaultKeyRequired() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return state_.required_user_action ==
             RequiredUserAction::kTrustedVaultKeyRequired ||
         state_.required_user_action ==
             RequiredUserAction::kTrustedVaultKeyRequiredButFetching;
}

bool SyncServiceCrypto::IsTrustedVaultRecoverabilityDegraded() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return state_.required_user_action ==
         RequiredUserAction::kTrustedVaultRecoverabilityDegraded;
}

void SyncServiceCrypto::EnableEncryptEverything() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(state_.engine);

  // TODO(atwilson): Persist the encryption_pending flag to address the various
  // problems around cancelling encryption in the background (crbug.com/119649).
  if (!state_.encrypt_everything)
    state_.encryption_pending = true;
}

bool SyncServiceCrypto::IsEncryptEverythingEnabled() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(state_.engine);
  return state_.encrypt_everything || state_.encryption_pending;
}

void SyncServiceCrypto::SetEncryptionPassphrase(const std::string& passphrase) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // This should only be called when the engine has been initialized.
  DCHECK(state_.engine);
  // We should never be called with an empty passphrase.
  DCHECK(!passphrase.empty());

  switch (state_.required_user_action) {
    case RequiredUserAction::kUnknownDuringInitialization:
    case RequiredUserAction::kNone:
    case RequiredUserAction::kTrustedVaultRecoverabilityDegraded:
      break;
    case RequiredUserAction::kPassphraseRequiredForDecryption:
    case RequiredUserAction::kFetchingTrustedVaultKeys:
    case RequiredUserAction::kTrustedVaultKeyRequired:
    case RequiredUserAction::kTrustedVaultKeyRequiredButFetching:
      // Cryptographer has pending keys.
      NOTREACHED()
          << "Can not set explicit passphrase when decryption is needed.";
      return;
    case RequiredUserAction::kPassphraseRequiredForEncryption:
      // |kPassphraseRequiredForEncryption| implies that the cryptographer does
      // not have pending keys. Hence, as long as we're not trying to do an
      // invalid passphrase change (e.g. explicit -> explicit or explicit ->
      // implicit), we know this will succeed. If for some reason a new
      // encryption key arrives via sync later, the SyncEncryptionHandler will
      // trigger another OnPassphraseRequired().
      UpdateRequiredUserActionAndNotify(RequiredUserAction::kNone);
      notify_observers_.Run();
      break;
  }

  DVLOG(1) << "Setting explicit passphrase for encryption.";

  // SetEncryptionPassphrase() should never be called if we are currently
  // encrypted with an explicit passphrase.
  DCHECK(!IsExplicitPassphrase(state_.cached_passphrase_type));

  state_.engine->SetEncryptionPassphrase(passphrase);
}

bool SyncServiceCrypto::SetDecryptionPassphrase(const std::string& passphrase) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // We should never be called with an empty passphrase.
  DCHECK(!passphrase.empty());

  // This should only be called when we have cached pending keys.
  DCHECK(state_.cached_pending_keys.has_blob());

  // For types other than CUSTOM_PASSPHRASE, we should be using the old PBKDF2
  // key derivation method.
  if (state_.cached_passphrase_type != PassphraseType::kCustomPassphrase) {
    DCHECK_EQ(state_.passphrase_key_derivation_params.method(),
              KeyDerivationMethod::PBKDF2_HMAC_SHA1_1003);
  }

  // Check the passphrase that was provided against our local cache of the
  // cryptographer's pending keys (which we cached during a previous
  // OnPassphraseRequired() event). If this was unsuccessful, the UI layer can
  // immediately call OnPassphraseRequired() again without showing the user a
  // spinner.
  if (!CheckPassphraseAgainstPendingKeys(
          state_.cached_pending_keys, state_.passphrase_key_derivation_params,
          passphrase)) {
    return false;
  }

  state_.engine->SetDecryptionPassphrase(passphrase);

  // Since we were able to decrypt the cached pending keys with the passphrase
  // provided, we immediately alert the UI layer that the passphrase was
  // accepted. This will avoid the situation where a user enters a passphrase,
  // clicks OK, immediately reopens the advanced settings dialog, and gets an
  // unnecessary prompt for a passphrase.
  // Note: It is not guaranteed that the passphrase will be accepted by the
  // syncer thread, since we could receive a new nigori node while the task is
  // pending. This scenario is a valid race, and SetDecryptionPassphrase() can
  // trigger a new OnPassphraseRequired() if it needs to.
  OnPassphraseAccepted();
  return true;
}

bool SyncServiceCrypto::IsTrustedVaultKeyRequiredStateKnown() const {
  switch (state_.required_user_action) {
    case RequiredUserAction::kUnknownDuringInitialization:
    case RequiredUserAction::kFetchingTrustedVaultKeys:
      return false;
    case RequiredUserAction::kNone:
    case RequiredUserAction::kPassphraseRequiredForDecryption:
    case RequiredUserAction::kTrustedVaultKeyRequired:
    case RequiredUserAction::kTrustedVaultKeyRequiredButFetching:
    case RequiredUserAction::kPassphraseRequiredForEncryption:
    case RequiredUserAction::kTrustedVaultRecoverabilityDegraded:
      return true;
  }
  NOTREACHED();
  return false;
}

PassphraseType SyncServiceCrypto::GetPassphraseType() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return state_.cached_passphrase_type;
}

ModelTypeSet SyncServiceCrypto::GetEncryptedDataTypes() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(state_.encrypted_types.Has(PASSWORDS));
  DCHECK(state_.encrypted_types.Has(WIFI_CONFIGURATIONS));
  // We may be called during the setup process before we're
  // initialized. In this case, we default to the sensitive types.
  return state_.encrypted_types;
}

bool SyncServiceCrypto::HasCryptoError() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // This determines whether DataTypeManager should issue crypto errors for
  // encrypted datatypes. This may differ from whether the UI represents the
  // error state or not.

  switch (state_.required_user_action) {
    case RequiredUserAction::kUnknownDuringInitialization:
    case RequiredUserAction::kNone:
    case RequiredUserAction::kTrustedVaultRecoverabilityDegraded:
      return false;
    case RequiredUserAction::kFetchingTrustedVaultKeys:
    case RequiredUserAction::kTrustedVaultKeyRequired:
    case RequiredUserAction::kTrustedVaultKeyRequiredButFetching:
    case RequiredUserAction::kPassphraseRequiredForDecryption:
    case RequiredUserAction::kPassphraseRequiredForEncryption:
      return true;
  }

  NOTREACHED();
  return false;
}

void SyncServiceCrypto::OnPassphraseRequired(
    PassphraseRequiredReason reason,
    const KeyDerivationParams& key_derivation_params,
    const sync_pb::EncryptedData& pending_keys) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Update our cache of the cryptographer's pending keys.
  state_.cached_pending_keys = pending_keys;

  // Update the key derivation params to be used.
  state_.passphrase_key_derivation_params = key_derivation_params;

  DVLOG(1) << "Passphrase required with reason: "
           << PassphraseRequiredReasonToString(reason);

  switch (reason) {
    case REASON_ENCRYPTION:
      UpdateRequiredUserActionAndNotify(
          RequiredUserAction::kPassphraseRequiredForEncryption);
      break;
    case REASON_DECRYPTION:
      UpdateRequiredUserActionAndNotify(
          RequiredUserAction::kPassphraseRequiredForDecryption);
      break;
  }

  // Reconfigure without the encrypted types (excluded implicitly via the
  // failed datatypes handler).
  reconfigure_.Run(CONFIGURE_REASON_CRYPTO);
}

void SyncServiceCrypto::OnPassphraseAccepted() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Clear our cache of the cryptographer's pending keys.
  state_.cached_pending_keys.clear_blob();

  // Reset |required_user_action| since we know we no longer require the
  // passphrase.
  UpdateRequiredUserActionAndNotify(RequiredUserAction::kNone);

  // Make sure the data types that depend on the passphrase are started at
  // this time.
  reconfigure_.Run(CONFIGURE_REASON_CRYPTO);
}

void SyncServiceCrypto::OnTrustedVaultKeyRequired() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // To be on the safe since, if a passphrase is required, we avoid overriding
  // |state_.required_user_action|.
  if (state_.required_user_action != RequiredUserAction::kNone &&
      state_.required_user_action !=
          RequiredUserAction::kUnknownDuringInitialization) {
    return;
  }

  UpdateRequiredUserActionAndNotify(
      RequiredUserAction::kFetchingTrustedVaultKeys);

  if (!state_.engine) {
    // If SetSyncEngine() hasn't been called yet, it means
    // OnTrustedVaultKeyRequired() was called as part of the engine's
    // initialization. Fetching the keys is not useful right now because there
    // is known engine to feed the keys to, so let's defer fetching until
    // SetSyncEngine() is called.
    return;
  }

  FetchTrustedVaultKeys(/*is_second_fetch_attempt=*/false);
}

void SyncServiceCrypto::OnTrustedVaultKeyAccepted() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  switch (state_.required_user_action) {
    case RequiredUserAction::kUnknownDuringInitialization:
    case RequiredUserAction::kNone:
    case RequiredUserAction::kPassphraseRequiredForDecryption:
    case RequiredUserAction::kPassphraseRequiredForEncryption:
    case RequiredUserAction::kTrustedVaultRecoverabilityDegraded:
      return;
    case RequiredUserAction::kFetchingTrustedVaultKeys:
    case RequiredUserAction::kTrustedVaultKeyRequired:
    case RequiredUserAction::kTrustedVaultKeyRequiredButFetching:
      break;
  }

  UpdateRequiredUserActionAndNotify(RequiredUserAction::kNone);

  // Make sure the data types that depend on the decryption key are started at
  // this time.
  reconfigure_.Run(CONFIGURE_REASON_CRYPTO);
}

void SyncServiceCrypto::OnBootstrapTokenUpdated(
    const std::string& bootstrap_token,
    BootstrapTokenType type) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(sync_prefs_);
  if (type == PASSPHRASE_BOOTSTRAP_TOKEN) {
    sync_prefs_->SetEncryptionBootstrapToken(bootstrap_token);
  } else {
    sync_prefs_->SetKeystoreEncryptionBootstrapToken(bootstrap_token);
  }
}

void SyncServiceCrypto::OnEncryptedTypesChanged(ModelTypeSet encrypted_types,
                                                bool encrypt_everything) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  state_.encrypted_types = encrypted_types;
  state_.encrypt_everything = encrypt_everything;
  DVLOG(1) << "Encrypted types changed to "
           << ModelTypeSetToString(state_.encrypted_types)
           << " (encrypt everything is set to "
           << (state_.encrypt_everything ? "true" : "false") << ")";
  DCHECK(state_.encrypted_types.Has(PASSWORDS));
  DCHECK(state_.encrypted_types.Has(WIFI_CONFIGURATIONS));

  notify_observers_.Run();
}

void SyncServiceCrypto::OnEncryptionComplete() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DVLOG(1) << "Encryption complete";
  if (state_.encryption_pending && state_.encrypt_everything) {
    state_.encryption_pending = false;
    // This is to nudge the integration tests when encryption is
    // finished.
    notify_observers_.Run();
  }
}

void SyncServiceCrypto::OnCryptographerStateChanged(
    Cryptographer* cryptographer,
    bool has_pending_keys) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Do nothing.
}

void SyncServiceCrypto::OnPassphraseTypeChanged(PassphraseType type,
                                                base::Time passphrase_time) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  DVLOG(1) << "Passphrase type changed to " << PassphraseTypeToString(type);

  state_.cached_passphrase_type = type;
  state_.cached_explicit_passphrase_time = passphrase_time;

  // Clear recoverability degraded state in case a custom passphrase was set.
  if (type != PassphraseType::kTrustedVaultPassphrase &&
      state_.required_user_action ==
          RequiredUserAction::kTrustedVaultRecoverabilityDegraded) {
    UpdateRequiredUserActionAndNotify(RequiredUserAction::kNone);
  }

  notify_observers_.Run();
}

void SyncServiceCrypto::SetSyncEngine(const CoreAccountInfo& account_info,
                                      SyncEngine* engine) {
  DCHECK(engine);
  state_.account_info = account_info;
  state_.engine = engine;

  // Since there was no state changes during engine initialization, now the
  // state is known and no user action required.
  if (state_.required_user_action ==
      RequiredUserAction::kUnknownDuringInitialization) {
    UpdateRequiredUserActionAndNotify(RequiredUserAction::kNone);
  }

  // This indicates OnTrustedVaultKeyRequired() was called as part of the
  // engine's initialization.
  if (state_.required_user_action ==
      RequiredUserAction::kFetchingTrustedVaultKeys) {
    FetchTrustedVaultKeys(/*is_second_fetch_attempt=*/false);
  }
}

std::unique_ptr<SyncEncryptionHandler::Observer>
SyncServiceCrypto::GetEncryptionObserverProxy() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return std::make_unique<SyncEncryptionObserverProxy>(
      weak_factory_.GetWeakPtr(), base::SequencedTaskRunnerHandle::Get());
}

void SyncServiceCrypto::OnTrustedVaultClientKeysChanged() {
  switch (state_.required_user_action) {
    case RequiredUserAction::kUnknownDuringInitialization:
    case RequiredUserAction::kNone:
    case RequiredUserAction::kPassphraseRequiredForDecryption:
    case RequiredUserAction::kPassphraseRequiredForEncryption:
    case RequiredUserAction::kTrustedVaultRecoverabilityDegraded:
      // If no trusted vault keys are required, there's nothing to do. If they
      // later are required, a fetch will be triggered in
      // OnTrustedVaultKeyRequired().
      return;
    case RequiredUserAction::kFetchingTrustedVaultKeys:
    case RequiredUserAction::kTrustedVaultKeyRequiredButFetching:
      // If there's an ongoing fetch, FetchKeys() cannot be issued immediately
      // since that violates the function precondition. However, the in-flight
      // FetchKeys() may end up returning stale keys, so let's make sure
      // FetchKeys() is invoked again once it becomes possible.
      state_.deferred_trusted_vault_fetch_keys_pending = true;
      return;
    case RequiredUserAction::kTrustedVaultKeyRequired:
      UpdateRequiredUserActionAndNotify(
          RequiredUserAction::kTrustedVaultKeyRequiredButFetching);
      break;
  }

  FetchTrustedVaultKeys(/*is_second_fetch_attempt=*/false);
}

void SyncServiceCrypto::FetchTrustedVaultKeys(bool is_second_fetch_attempt) {
  DCHECK(state_.engine);
  DCHECK(state_.required_user_action ==
             RequiredUserAction::kFetchingTrustedVaultKeys ||
         state_.required_user_action ==
             RequiredUserAction::kTrustedVaultKeyRequiredButFetching);

  UMA_HISTOGRAM_ENUMERATION(
      "Sync.TrustedVaultFetchKeysAttempt",
      is_second_fetch_attempt
          ? TrustedVaultFetchKeysAttemptForUMA::kSecondAttempt
          : TrustedVaultFetchKeysAttemptForUMA::kFirstAttempt);

  if (!is_second_fetch_attempt) {
    state_.deferred_trusted_vault_fetch_keys_pending = false;
  }

  trusted_vault_client_->FetchKeys(
      state_.account_info,
      base::BindOnce(&SyncServiceCrypto::TrustedVaultKeysFetchedFromClient,
                     weak_factory_.GetWeakPtr(), is_second_fetch_attempt));
}

void SyncServiceCrypto::TrustedVaultKeysFetchedFromClient(
    bool is_second_fetch_attempt,
    const std::vector<std::vector<uint8_t>>& keys) {
  if (state_.required_user_action !=
          RequiredUserAction::kFetchingTrustedVaultKeys &&
      state_.required_user_action !=
          RequiredUserAction::kTrustedVaultKeyRequiredButFetching) {
    return;
  }

  DCHECK(state_.engine);

  UMA_HISTOGRAM_COUNTS_100("Sync.TrustedVaultFetchedKeysCount", keys.size());

  if (keys.empty()) {
    // Nothing to do if no keys have been fetched from the client (e.g. user
    // action is required for fetching additional keys). Let's avoid unnecessary
    // steps like marking keys as stale.
    FetchTrustedVaultKeysCompletedButInsufficient();
    return;
  }

  state_.engine->AddTrustedVaultDecryptionKeys(
      keys,
      base::BindOnce(&SyncServiceCrypto::TrustedVaultKeysAdded,
                     weak_factory_.GetWeakPtr(), is_second_fetch_attempt));
}

void SyncServiceCrypto::TrustedVaultKeysAdded(bool is_second_fetch_attempt) {
  // Having kFetchingTrustedVaultKeys or kTrustedVaultKeyRequiredButFetching
  // indicates OnTrustedVaultKeyAccepted() was not triggered, so the fetched
  // trusted vault keys were insufficient.
  bool success = state_.required_user_action !=
                     RequiredUserAction::kFetchingTrustedVaultKeys &&
                 state_.required_user_action !=
                     RequiredUserAction::kTrustedVaultKeyRequiredButFetching;

  UMA_HISTOGRAM_BOOLEAN("Sync.TrustedVaultAddKeysAttemptIsSuccessful", success);

  if (success) {
    return;
  }

  // Let trusted vault client know, that fetched keys were insufficient.
  trusted_vault_client_->MarkKeysAsStale(
      state_.account_info,
      base::BindOnce(&SyncServiceCrypto::TrustedVaultKeysMarkedAsStale,
                     weak_factory_.GetWeakPtr(), is_second_fetch_attempt));
}

void SyncServiceCrypto::TrustedVaultKeysMarkedAsStale(
    bool is_second_fetch_attempt,
    bool result) {
  if (state_.required_user_action !=
          RequiredUserAction::kFetchingTrustedVaultKeys &&
      state_.required_user_action !=
          RequiredUserAction::kTrustedVaultKeyRequiredButFetching) {
    return;
  }

  // If nothing has changed (determined by |!result| since false negatives are
  // disallowed by the API) or this is already a second attempt, the fetching
  // procedure can be considered completed.
  if (!result || is_second_fetch_attempt) {
    FetchTrustedVaultKeysCompletedButInsufficient();
    return;
  }

  FetchTrustedVaultKeys(/*is_second_fetch_attempt=*/true);
}

void SyncServiceCrypto::FetchTrustedVaultKeysCompletedButInsufficient() {
  DCHECK(state_.required_user_action ==
             RequiredUserAction::kFetchingTrustedVaultKeys ||
         state_.required_user_action ==
             RequiredUserAction::kTrustedVaultKeyRequiredButFetching);

  // If FetchKeys() was intended to be called during an already existing ongoing
  // FetchKeys(), it needs to be invoked now that it's possible.
  if (state_.deferred_trusted_vault_fetch_keys_pending) {
    FetchTrustedVaultKeys(/*is_second_fetch_attempt=*/false);
    return;
  }

  // Reaching this codepath indicates OnTrustedVaultKeyAccepted() was not
  // triggered, so the fetched trusted vault keys were insufficient.
  UpdateRequiredUserActionAndNotify(
      RequiredUserAction::kTrustedVaultKeyRequired);

  // Reconfigure without the encrypted types (excluded implicitly via the failed
  // datatypes handler).
  reconfigure_.Run(CONFIGURE_REASON_CRYPTO);
}

void SyncServiceCrypto::UpdateRequiredUserActionAndNotify(
    RequiredUserAction new_required_user_action) {
  DCHECK_NE(new_required_user_action,
            RequiredUserAction::kUnknownDuringInitialization);

  if (state_.required_user_action == new_required_user_action) {
    return;
  }

  state_.required_user_action = new_required_user_action;
  notify_required_user_action_changed_.Run();

  if (state_.required_user_action == RequiredUserAction::kNone &&
      state_.cached_passphrase_type ==
          PassphraseType::kTrustedVaultPassphrase &&
      base::FeatureList::IsEnabled(
          switches::kSyncSupportTrustedVaultPassphraseRecovery)) {
    trusted_vault_client_->GetIsRecoverabilityDegraded(
        state_.account_info,
        base::BindOnce(&SyncServiceCrypto::GetIsRecoverabilityDegradedCompleted,
                       weak_factory_.GetWeakPtr()));
  }
}

void SyncServiceCrypto::GetIsRecoverabilityDegradedCompleted(
    bool is_recoverability_degraded) {
  if (state_.cached_passphrase_type !=
      PassphraseType::kTrustedVaultPassphrase) {
    DCHECK_NE(state_.required_user_action,
              RequiredUserAction::kTrustedVaultRecoverabilityDegraded);
    return;
  }

  // Transition from non-degraded to degraded recoverability.
  if (is_recoverability_degraded &&
      state_.required_user_action == RequiredUserAction::kNone) {
    UpdateRequiredUserActionAndNotify(
        RequiredUserAction::kTrustedVaultRecoverabilityDegraded);
    notify_observers_.Run();
  }

  // Transition from degraded to non-degraded recoverability.
  if (!is_recoverability_degraded &&
      state_.required_user_action ==
          RequiredUserAction::kTrustedVaultRecoverabilityDegraded) {
    UpdateRequiredUserActionAndNotify(RequiredUserAction::kNone);
    notify_observers_.Run();
  }
}

}  // namespace syncer