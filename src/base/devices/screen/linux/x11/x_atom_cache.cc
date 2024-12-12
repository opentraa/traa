/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/linux/x11/x_atom_cache.h"

namespace traa {
namespace base {

x_atom_cache::x_atom_cache(::Display *display) : display_(display) {}

x_atom_cache::~x_atom_cache() = default;

::Display *x_atom_cache::display() const { return display_; }

Atom x_atom_cache::wm_state() { return create_if_not_exist(&wm_state_, "WM_STATE"); }

Atom x_atom_cache::window_type() {
  return create_if_not_exist(&window_type_, "_NET_WM_WINDOW_TYPE");
}

Atom x_atom_cache::window_type_normal() {
  return create_if_not_exist(&window_type_normal_, "_NET_WM_WINDOW_TYPE_NORMAL");
}

Atom x_atom_cache::icc_profile() { return create_if_not_exist(&icc_profile_, "_ICC_PROFILE"); }

Atom x_atom_cache::create_if_not_exist(Atom *atom, const char *name) {
  if (*atom == None) {
    *atom = XInternAtom(display(), name, True);
  }
  return *atom;
}

} // namespace base
} // namespace traa