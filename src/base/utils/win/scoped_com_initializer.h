/*
 *  Copyright (c) 2020 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_UTILS_WIN_SCOPED_COM_INITIALIZER_H_
#define TRAA_BASE_UTILS_WIN_SCOPED_COM_INITIALIZER_H_

#include <comdef.h>

namespace traa {
namespace base {

// Initializes COM in the constructor (STA or MTA), and uninitializes COM in the
// destructor. Taken from traa::base::scoped_com_initializer.
//
// WARNING: This should only be used once per thread, ideally scoped to a
// similar lifetime as the thread itself.  You should not be using this in
// random utility functions that make COM calls; instead ensure that these
// functions are running on a COM-supporting thread!
// See https://msdn.microsoft.com/en-us/library/ms809971.aspx for details.
class scoped_com_initializer {
public:
  // Enum value provided to initialize the thread as an MTA instead of STA.
  // There are two types of apartments, Single Threaded Apartments (STAs)
  // and Multi Threaded Apartments (MTAs). Within a given process there can
  // be multiple STA’s but there is only one MTA. STA is typically used by
  // "GUI applications" and MTA by "worker threads" with no UI message loop.
  enum select_mta { SELECT_MTA };

  // Constructor for STA initialization.
  scoped_com_initializer();

  // Constructor for MTA initialization.
  explicit scoped_com_initializer(select_mta mta);

  ~scoped_com_initializer();

  scoped_com_initializer(const scoped_com_initializer &) = delete;
  scoped_com_initializer &operator=(const scoped_com_initializer &) = delete;

  bool succeeded() { return SUCCEEDED(hr_); }

private:
  void initialize(COINIT init);

  HRESULT hr_;
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_UTILS_WIN_SCOPED_COM_INITIALIZER_H_