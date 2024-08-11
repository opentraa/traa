#ifndef TRAA_BASE_OS_WIN_VERSION_H_
#define TRAA_BASE_OS_WIN_VERSION_H_

#include <Windows.h>

#include <stddef.h>
#include <string>

namespace traa {
namespace base {

// The running version of Windows.  This is declared outside os_info for
// syntactic sugar reasons;
//
// This enum is used in metrics histograms, so they shouldn't be reordered or
// removed. New values can be added before VERSION_WIN_LAST.
typedef enum version_alias {
  VERSION_PRE_XP = 0, // Not supported.
  VERSION_XP = 1,
  VERSION_SERVER_2003 = 2,  // Also includes XP Pro x64 and Server 2003 R2.
  VERSION_VISTA = 3,        // Also includes Windows Server 2008.
  VERSION_WIN7 = 4,         // Also includes Windows Server 2008 R2.
  VERSION_WIN8 = 5,         // Also includes Windows Server 2012.
  VERSION_WIN8_1 = 6,       // Also includes Windows Server 2012 R2.
  VERSION_WIN10 = 7,        // Threshold 1: Version 1507, Build 10240.
  VERSION_WIN10_TH2 = 8,    // Threshold 2: Version 1511, Build 10586.
  VERSION_WIN10_RS1 = 9,    // Redstone 1: Version 1607, Build 14393.
  VERSION_WIN10_RS2 = 10,   // Redstone 2: Version 1703, Build 15063.
  VERSION_WIN10_RS3 = 11,   // Redstone 3: Version 1709, Build 16299.
  VERSION_WIN10_RS4 = 12,   // Redstone 4: Version 1803, Build 17134.
  VERSION_WIN10_RS5 = 13,   // Redstone 5: Version 1809, Build 17763.
  VERSION_WIN10_19H1 = 14,  // 19H1: Version 1903, Build 18362.
  VERSION_WIN10_19H2 = 15,  // 19H2: Version 1909, Build 18363.
  VERSION_WIN10_20H1 = 16,  // 20H1: Version 2004, Build 19041.
  VERSION_WIN10_20H2 = 17,  // 20H2: Build 19042.
  VERSION_WIN10_21H1 = 18,  // 21H1: Build 19043.
  VERSION_WIN10_21H2 = 19,  // 21H2: Build 19044.
  VERSION_SERVER_2022 = 20, // Server 2022: Build 20348.
  VERSION_WIN11 = 21,       // Windows 11: Build 22000.
  VERSION_WIN_LAST,         // Indicates error condition.
} version_alias;

// A rough bucketing of the available types of versions of Windows. This is used
// to distinguish enterprise enabled versions from home versions and potentially
// server versions. Keep these values in the same order, since they are used as
// is for metrics histogram ids.
enum version_type {
  SUITE_HOME = 0,
  SUITE_PROFESSIONAL,
  SUITE_SERVER,
  SUITE_ENTERPRISE,
  SUITE_EDUCATION,
  SUITE_LAST,
};

// A singleton that can be used to query various pieces of information about the
// OS and process state. Note that this doesn't use the base Singleton class, so
// it can be used without an AtExitManager.
class os_info {
public:
  struct version_number {
    int major;
    int minor;
    int build;
    int patch;
  };

  struct service_pack {
    int major;
    int minor;
  };

  // The processor architecture this copy of Windows natively uses.  For
  // example, given an x64-capable processor, we have three possibilities:
  //   32-bit Chrome running on 32-bit Windows:           X86_ARCHITECTURE
  //   32-bit Chrome running on 64-bit Windows via WOW64: X64_ARCHITECTURE
  //   64-bit Chrome running on 64-bit Windows:           X64_ARCHITECTURE
  enum win_architecture {
    X86_ARCHITECTURE,
    X64_ARCHITECTURE,
    IA64_ARCHITECTURE,
    OTHER_ARCHITECTURE,
  };

  // Whether a process is running under WOW64 (the wrapper that allows 32-bit
  // processes to run on 64-bit versions of Windows).  This will return
  // WOW64_DISABLED for both "32-bit Chrome on 32-bit Windows" and "64-bit
  // Chrome on 64-bit Windows".  WOW64_UNKNOWN means "an error occurred", e.g.
  // the process does not have sufficient access rights to determine this.
  enum wow64_status {
    WOW64_DISABLED,
    WOW64_ENABLED,
    WOW64_UNKNOWN,
  };

  os_info(const os_info &) = delete;
  os_info &operator=(const os_info &) = delete;

  static os_info *instance();

  version_alias version() const { return version_; }
  version_number number() const { return number_; }
  version_type type() const { return type_; }
  service_pack pack() const { return pack_; }
  std::string service_pack_str() const { return service_pack_str_; }
  win_architecture architecture() const { return architecture_; }
  int processors() const { return processors_; }
  size_t allocation_granularity() const { return allocation_granularity_; }
  wow64_status wow64() const { return wow64_; }
  std::string processor_model_name();

  // Like wow64_status(), but for the supplied handle instead of the current
  // process.  This doesn't touch member state, so you can bypass the singleton.
  static wow64_status get_wow64_status(HANDLE process_handle);

private:
  os_info();
  ~os_info();

  version_alias version_;
  version_number number_;
  version_type type_;
  service_pack pack_;

  // A string, such as "Service Pack 3", that indicates the latest Service Pack
  // installed on the system. If no Service Pack has been installed, the string
  // is empty.
  std::string service_pack_str_;
  win_architecture architecture_;
  int processors_;
  size_t allocation_granularity_;
  wow64_status wow64_;
  std::string processor_model_name_;
};

// Because this is by far the most commonly-requested value from the above
// singleton, we add a global-scope accessor here as syntactic sugar.
version_alias get_version();

bool is_win_xp_or_later() { return get_version() >= VERSION_XP; }

bool is_win_vista_or_later() { return get_version() >= VERSION_VISTA; }

bool is_win_7_or_later() { return get_version() >= VERSION_WIN7; }

bool is_win_8_or_later() { return get_version() >= VERSION_WIN8; }

bool is_win_8_1_or_later() { return get_version() >= VERSION_WIN8_1; }

bool is_win_10_or_later() { return get_version() >= VERSION_WIN10; }

bool is_win_11_or_later() { return get_version() >= VERSION_WIN11; }

} // namespace base
} // namespace traa

#endif // TRAA_BASE_OS_WIN_VERSION_H_
