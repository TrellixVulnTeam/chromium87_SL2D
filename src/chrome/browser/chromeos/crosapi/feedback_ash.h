// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_CROSAPI_FEEDBACK_ASH_H_
#define CHROME_BROWSER_CHROMEOS_CROSAPI_FEEDBACK_ASH_H_

#include "chromeos/crosapi/mojom/feedback.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace crosapi {

// Implements the crosapi feedback interface. Lives in ash-chrome on the
// UI thread. Shows feedback page in response to mojo IPCs from lacros-chrome.
class FeedbackAsh : public mojom::Feedback {
 public:
  explicit FeedbackAsh(mojo::PendingReceiver<mojom::Feedback> receiver);
  FeedbackAsh(const FeedbackAsh&) = delete;
  FeedbackAsh& operator=(const FeedbackAsh&) = delete;
  ~FeedbackAsh() override;

  // crosapi::mojom::Feedback:
  void ShowFeedbackPage(mojom::FeedbackInfoPtr feedback_info) override;

 private:
  mojo::Receiver<mojom::Feedback> receiver_;
};

}  // namespace crosapi

#endif  // CHROME_BROWSER_CHROMEOS_CROSAPI_FEEDBACK_ASH_H_