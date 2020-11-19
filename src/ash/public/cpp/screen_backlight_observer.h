// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_PUBLIC_CPP_SCREEN_BACKLIGHT_OBSERVER_H_
#define ASH_PUBLIC_CPP_SCREEN_BACKLIGHT_OBSERVER_H_

#include "ash/public/cpp/ash_public_export.h"
#include "ash/public/cpp/screen_backlight_type.h"

namespace ash {

// Used to observe backlight related events inside ash.
class ASH_PUBLIC_EXPORT ScreenBacklightObserver {
 public:
  virtual ~ScreenBacklightObserver() = default;

  // Called when the observed BacklightsForcedOffSetter instance stops or
  // starts forcing backlights off.
  virtual void OnBacklightsForcedOffChanged(bool backlights_forced_off) {}

  // Called when the screen state change is detected.
  virtual void OnScreenStateChanged(ScreenState screen_state) {}
};

}  // namespace ash

#endif  // ASH_PUBLIC_CPP_SCREEN_BACKLIGHT_OBSERVER_H_