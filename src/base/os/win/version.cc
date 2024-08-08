/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/os/win/version.h"
#include "base/strings/string_trans.h"

#include <memory>

#if !defined(__clang__) && _MSC_FULL_VER < 191125507
#error VS 2017 Update 3.2 or higher is required
#endif

#if !defined(WINUWP)

namespace {

typedef BOOL(WINAPI *GetProductInfoPtr)(DWORD, DWORD, DWORD, DWORD, PDWORD);

// Mask to pull WOW64 access flags out of REGSAM access.
const REGSAM kWow64AccessMask = KEY_WOW64_32KEY | KEY_WOW64_64KEY;

// Utility class to read, write and manipulate the Windows Registry.
// Registry vocabulary primer: a "key" is like a folder, in which there
// are "values", which are <name, data> pairs, with an associated data type.
class registry {
public:
  registry() : key_(nullptr), wow64access_(0) {}

  registry(HKEY rootkey, const wchar_t *subkey, REGSAM access) : key_(nullptr), wow64access_(0) {
    if (rootkey) {
      if (access & (KEY_SET_VALUE | KEY_CREATE_SUB_KEY | KEY_CREATE_LINK))
        create(rootkey, subkey, access);
      else
        open(rootkey, subkey, access);
    } else {
      wow64access_ = access & kWow64AccessMask;
    }
  }

  ~registry() { close(); }

  LONG create(HKEY rootkey, const wchar_t *subkey, REGSAM access) {
    DWORD disposition_value;
    return create_with_disposiion(rootkey, subkey, &disposition_value, access);
  }

  LONG create_with_disposiion(HKEY rootkey, const wchar_t *subkey, DWORD *disposition,
                              REGSAM access) {
    HKEY subhkey = NULL;
    LONG result = ::RegCreateKeyExW(rootkey, subkey, 0, NULL, REG_OPTION_NON_VOLATILE, access, NULL,
                                    &subhkey, disposition);
    if (result == ERROR_SUCCESS) {
      close();
      key_ = subhkey;
      wow64access_ = access & kWow64AccessMask;
    }

    return result;
  }

  // Opens an existing reg key.
  LONG open(HKEY rootkey, const wchar_t *subkey, REGSAM access) {
    HKEY subhkey = NULL;

    LONG result = ::RegOpenKeyExW(rootkey, subkey, 0, access, &subhkey);
    if (result == ERROR_SUCCESS) {
      close();
      key_ = subhkey;
      wow64access_ = access & kWow64AccessMask;
    }

    return result;
  }

  // Closes this reg key.
  void close() {
    if (key_) {
      ::RegCloseKey(key_);
      key_ = nullptr;
    }
  }

  // Reads a REG_DWORD (uint32_t) into `out_value`. If `name` is null or empty,
  // reads the key's default value, if any.
  LONG read(const wchar_t *name, DWORD *out_value) const {
    DWORD type = REG_DWORD;
    DWORD size = sizeof(DWORD);
    DWORD local_value = 0;
    LONG result = read(name, &local_value, &size, &type);
    if (result == ERROR_SUCCESS) {
      if ((type == REG_DWORD || type == REG_BINARY) && size == sizeof(DWORD))
        *out_value = local_value;
      else
        result = ERROR_CANTREAD;
    }

    return result;
  }

  // Reads a string into `out_value`. If `name` is null or empty, reads
  // the key's default value, if any.
  LONG read(const wchar_t *name, std::wstring &out_value) const {
    const size_t kMaxStringLength = 1024; // This is after expansion.
    // Use the one of the other forms of read if 1024 is too small for you.
    wchar_t raw_value[kMaxStringLength];
    DWORD type = REG_SZ, size = sizeof(raw_value);
    LONG result = read(name, raw_value, &size, &type);
    if (result == ERROR_SUCCESS) {
      if (type == REG_SZ) {
        out_value = raw_value;
      } else if (type == REG_EXPAND_SZ) {
        wchar_t expanded[kMaxStringLength];
        size = ::ExpandEnvironmentStringsW(raw_value, expanded, kMaxStringLength);
        // Success: returns the number of wchar_t's copied
        // Fail: buffer too small, returns the size required
        // Fail: other, returns 0
        if (size == 0 || size > kMaxStringLength) {
          result = ERROR_MORE_DATA;
        } else {
          out_value = expanded;
        }
      } else {
        // Not a string. Oops.
        result = ERROR_CANTREAD;
      }
    }

    return result;
  }

  LONG read(const wchar_t *name, void *data, DWORD *dsize, DWORD *dtype) const {
    LONG result = RegQueryValueExW(key_, name, 0, dtype, reinterpret_cast<LPBYTE>(data), dsize);
    return result;
  }

private:
  HKEY key_;
  REGSAM wow64access_;
};

// Helper to map a major.minor.x.build version (e.g. 6.1) to a Windows release.
version_alias major_minor_build_to_version(int major, int minor, int build) {
  if ((major == 5) && (minor > 0)) {
    // Treat XP Pro x64, Home Server, and Server 2003 R2 as Server 2003.
    return (minor == 1) ? VERSION_XP : VERSION_SERVER_2003;
  } else if (major == 6) {
    switch (minor) {
    case 0:
      // Treat Windows Server 2008 the same as Windows Vista.
      return VERSION_VISTA;
    case 1:
      // Treat Windows Server 2008 R2 the same as Windows 7.
      return VERSION_WIN7;
    case 2:
      // Treat Windows Server 2012 the same as Windows 8.
      return VERSION_WIN8;
    default:
      return VERSION_WIN8_1;
    }
  } else if (major == 10) {
    if (build < 10586) {
      return VERSION_WIN10;
    } else if (build < 14393) {
      return VERSION_WIN10_TH2;
    } else if (build < 15063) {
      return VERSION_WIN10_RS1;
    } else if (build < 16299) {
      return VERSION_WIN10_RS2;
    } else if (build < 17134) {
      return VERSION_WIN10_RS3;
    } else if (build < 17763) {
      return VERSION_WIN10_RS4;
    } else if (build < 18362) {
      return VERSION_WIN10_RS5;
    } else if (build < 18363) {
      return VERSION_WIN10_19H1;
    } else if (build < 19041) {
      return VERSION_WIN10_19H2;
    } else if (build < 19042) {
      return VERSION_WIN10_20H1;
    } else if (build < 19043) {
      return VERSION_WIN10_20H2;
    } else if (build < 19044) {
      return VERSION_WIN10_21H1;
    } else if (build < 20348) {
      return VERSION_WIN10_21H2;
    } else if (build < 22000) {
      return VERSION_SERVER_2022;
    } else {
      return VERSION_WIN11;
    }
  } else if (major == 11) {
    return VERSION_WIN11;
  } else if (major > 6) {
    return VERSION_WIN_LAST;
  }

  return VERSION_PRE_XP;
}

// Returns the the "UBR" value from the registry. Introduced in Windows 10,
// this undocumented value appears to be similar to a patch number.
// Returns 0 if the value does not exist or it could not be read.
int get_ubr() {
#if defined(WINUWP)
  // The registry is not accessible for WinUWP sandboxed store applications.
  return 0;
#else
  // The values under the CurrentVersion registry hive are mirrored under
  // the corresponding Wow6432 hive.
  static constexpr wchar_t kRegKeyWindowsNTCurrentVersion[] =
      L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";

  registry key;
  if (key.open(HKEY_LOCAL_MACHINE, kRegKeyWindowsNTCurrentVersion, KEY_QUERY_VALUE) !=
      ERROR_SUCCESS) {
    return 0;
  }

  DWORD ubr = 0;
  key.read(L"UBR", &ubr);

  return static_cast<int>(ubr);
#endif // defined(WINUWP)
}

bool internal_get_version_numbers(OSVERSIONINFOW *osvi) {
  if (osvi == nullptr)
    return false;

  HINSTANCE hins = LoadLibraryA("ntdll.dll");
  if (!hins) {
#pragma warning(push)
#pragma warning(disable : 4996)
    // If we can't load ntdll.dll, fall back to GetVersionEx.
    return GetVersionExW(osvi);
#pragma warning(pop)
  }

  auto ntproc =
      (void(__stdcall *)(DWORD *, DWORD *, DWORD *))GetProcAddress(hins, "RtlGetNtVersionNumbers");
  if (ntproc) {
    ntproc(&osvi->dwMajorVersion, &osvi->dwMinorVersion, &osvi->dwBuildNumber);
    osvi->dwBuildNumber &= 0xFFFF; // Win32 build number is in low word.
    osvi->dwPlatformId = VER_PLATFORM_WIN32_NT;
  }

  FreeLibrary(hins);

  return ntproc != nullptr;
}

} // namespace

#endif // !defined(WINUWP)

namespace traa {
namespace base {

// static
os_info *os_info::instance() {
  // Note: we don't use the Singleton class because it depends on AtExitManager,
  // and it's convenient for other modules to use this class without it. This
  // pattern is copied from gurl.cc.
  static os_info *info;
  if (!info) {
    os_info *new_info = new os_info();
    if (InterlockedCompareExchangePointer(reinterpret_cast<PVOID *>(&info), new_info, NULL)) {
      delete new_info;
    }
  }
  return info;
}

os_info::os_info()
    : version_(VERSION_PRE_XP), architecture_(OTHER_ARCHITECTURE),
      wow64_(get_wow64_status(GetCurrentProcess())) {
  // Applications not manifested for Windows 8.1 or Windows 10 will return the
  // Windows 8 OS version value (6.2). Once an application is manifested for a
  // given operating system version, GetVersionEx() will always return the
  // version that the application is manifested for in future releases.
  //
  // After all, we decide to use the undocumented function RtlGetNtVersionNumbers to get the real
  // version.
  //
  // https://docs.microsoft.com/en-us/windows/desktop/SysInfo/targeting-your-application-at-windows-8-1
  // https://www.codeproject.com/Articles/678606/Part-Overcoming-Windows-s-deprecation-of-GetVe
  // https://www.geoffchappell.com/studies/windows/win32/ntdll/api/ldrinit/getntversionnumbers.htm
  //
  OSVERSIONINFOEXW version_info = {sizeof version_info};
  internal_get_version_numbers(reinterpret_cast<OSVERSIONINFOW *>(&version_info));

  number_.major = version_info.dwMajorVersion;
  number_.minor = version_info.dwMinorVersion;
  number_.build = version_info.dwBuildNumber;
  number_.patch = get_ubr();

  version_ = major_minor_build_to_version(number_.major, number_.minor, number_.build);

  pack_.major = version_info.wServicePackMajor;
  pack_.minor = version_info.wServicePackMinor;

  service_pack_str_ = traa::base::string_trans::unicode_to_utf8(version_info.szCSDVersion);

  SYSTEM_INFO system_info = {};
  ::GetNativeSystemInfo(&system_info);
  switch (system_info.wProcessorArchitecture) {
  case PROCESSOR_ARCHITECTURE_INTEL:
    architecture_ = X86_ARCHITECTURE;
    break;
  case PROCESSOR_ARCHITECTURE_AMD64:
    architecture_ = X64_ARCHITECTURE;
    break;
  case PROCESSOR_ARCHITECTURE_IA64:
    architecture_ = IA64_ARCHITECTURE;
    break;
  }
  processors_ = system_info.dwNumberOfProcessors;
  allocation_granularity_ = system_info.dwAllocationGranularity;

#if !defined(WINUWP)
  GetProductInfoPtr get_product_info;
  DWORD os_type;

  if (version_info.dwMajorVersion == 6 || version_info.dwMajorVersion == 10) {
    // Only present on Vista+.
    get_product_info = reinterpret_cast<GetProductInfoPtr>(
        ::GetProcAddress(::GetModuleHandleW(L"kernel32.dll"), "GetProductInfo"));

    get_product_info(version_info.dwMajorVersion, version_info.dwMinorVersion, 0, 0, &os_type);
    switch (os_type) {
    case PRODUCT_CLUSTER_SERVER:
    case PRODUCT_DATACENTER_SERVER:
    case PRODUCT_DATACENTER_SERVER_CORE:
    case PRODUCT_ENTERPRISE_SERVER:
    case PRODUCT_ENTERPRISE_SERVER_CORE:
    case PRODUCT_ENTERPRISE_SERVER_IA64:
    case PRODUCT_SMALLBUSINESS_SERVER:
    case PRODUCT_SMALLBUSINESS_SERVER_PREMIUM:
    case PRODUCT_STANDARD_SERVER:
    case PRODUCT_STANDARD_SERVER_CORE:
    case PRODUCT_WEB_SERVER:
      type_ = SUITE_SERVER;
      break;
    case PRODUCT_PROFESSIONAL:
    case PRODUCT_ULTIMATE:
      type_ = SUITE_PROFESSIONAL;
      break;
    case PRODUCT_ENTERPRISE:
    case PRODUCT_ENTERPRISE_E:
    case PRODUCT_ENTERPRISE_EVALUATION:
    case PRODUCT_ENTERPRISE_N:
    case PRODUCT_ENTERPRISE_N_EVALUATION:
    case PRODUCT_ENTERPRISE_S:
    case PRODUCT_ENTERPRISE_S_EVALUATION:
    case PRODUCT_ENTERPRISE_S_N:
    case PRODUCT_ENTERPRISE_S_N_EVALUATION:
    case PRODUCT_BUSINESS:
    case PRODUCT_BUSINESS_N:
      type_ = SUITE_ENTERPRISE;
      break;
    case PRODUCT_EDUCATION:
    case PRODUCT_EDUCATION_N:
      type_ = SUITE_EDUCATION;
      break;
    case PRODUCT_HOME_BASIC:
    case PRODUCT_HOME_PREMIUM:
    case PRODUCT_STARTER:
    default:
      type_ = SUITE_HOME;
      break;
    }
  } else if (version_info.dwMajorVersion == 5 && version_info.dwMinorVersion == 2) {
    if (version_info.wProductType == VER_NT_WORKSTATION &&
        system_info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) {
      type_ = SUITE_PROFESSIONAL;
    } else if (version_info.wSuiteMask & VER_SUITE_WH_SERVER) {
      type_ = SUITE_HOME;
    } else {
      type_ = SUITE_SERVER;
    }
  } else if (version_info.dwMajorVersion == 5 && version_info.dwMinorVersion == 1) {
    if (version_info.wSuiteMask & VER_SUITE_PERSONAL)
      type_ = SUITE_HOME;
    else
      type_ = SUITE_PROFESSIONAL;
  } else {
    // Windows is pre XP so we don't care but pick a safe default.
    type_ = SUITE_HOME;
  }
#else
  // WinUWP sandboxed store apps do not have a mechanism to determine
  // product suite thus the most restricted suite is chosen.
  type_ = SUITE_HOME;
#endif // !defined(WINUWP)
}

os_info::~os_info() {}

std::string os_info::processor_model_name() {
#if defined(WINUWP)
  // WinUWP sandboxed store apps do not have the ability to
  // probe the name of the current processor.
  return "Unknown Processor (UWP)";
#else
  if (processor_model_name_.empty()) {
    const wchar_t kProcessorNameString[] = L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0";
    registry key(HKEY_LOCAL_MACHINE, kProcessorNameString, KEY_READ);
    std::wstring value;
    key.read(L"ProcessorNameString", value);
    processor_model_name_ = traa::base::string_trans::unicode_to_utf8(value);
  }
  return processor_model_name_;
#endif // defined(WINUWP)
}

// static
os_info::wow64_status os_info::get_wow64_status(HANDLE process_handle) {
  BOOL is_wow64;
#if defined(WINUWP)
  if (!IsWow64Process(process_handle, &is_wow64))
    return WOW64_UNKNOWN;
#else
  typedef BOOL(WINAPI * IsWow64ProcessFunc)(HANDLE, PBOOL);
  IsWow64ProcessFunc is_wow64_process = reinterpret_cast<IsWow64ProcessFunc>(
      GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "IsWow64Process"));
  if (!is_wow64_process)
    return WOW64_DISABLED;
  if (!(*is_wow64_process)(process_handle, &is_wow64))
    return WOW64_UNKNOWN;
#endif // defined(WINUWP)
  return is_wow64 ? WOW64_ENABLED : WOW64_DISABLED;
}

version_alias get_version() { return os_info::instance()->version(); }

} // namespace base
} // namespace traa
