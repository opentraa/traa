#include "base/utils/win/registry.h"

#include <algorithm>
#include <assert.h>
#include <shlwapi.h>

// ntstatus.h conflicts with windows.h so define this locally.
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#define STATUS_NOT_FOUND ((NTSTATUS)0xC0000225L)

namespace traa {
namespace base {

inline namespace {

// RegEnumValue() reports the number of characters from the name that were
// written to the buffer, not how many there are. This constant is the maximum
// name size, such that a buffer with this size should read any name.
constexpr DWORD k_max_registry_name_size = 16384;

// Registry values are read as BYTE* but can have wchar_t* data whose last
// wchar_t is truncated. This function converts the reported |byte_size| to
// a size in wchar_t that can store a truncated wchar_t if necessary.
inline DWORD to_wchar_size(DWORD byte_size) {
  return (byte_size + sizeof(wchar_t) - 1) / sizeof(wchar_t);
}

// Mask to pull WOW64 access flags out of REGSAM access.
constexpr REGSAM k_wow64_access_mask = KEY_WOW64_32KEY | KEY_WOW64_64KEY;

constexpr DWORD k_invalid_iter_value = static_cast<DWORD>(-1);

// Reserves enough memory in |str| to accommodate |length_with_null| characters,
// sets the size of |str| to |length_with_null - 1| characters, and returns a
// pointer to the underlying contiguous array of characters.  This is typically
// used when calling a function that writes results into a character array, but
// the caller wants the data to be managed by a string-like object.  It is
// convenient in that is can be used inline in the call, and fast in that it
// avoids copying the results of the call from a char* into a string.
//
// |length_with_null| must be at least 2, since otherwise the underlying string
// would have size 0, and trying to access &((*str)[0]) in that case can result
// in a number of problems.
//
// Internally, this takes linear time because the resize() call 0-fills the
// underlying array for potentially all
// (|length_with_null - 1| * sizeof(string_type::value_type)) bytes.  Ideally we
// could avoid this aspect of the resize() call, as we expect the caller to
// immediately write over this memory, but there is no other way to set the size
// of the string, and not doing that will mean people who access |str| rather
// than str.c_str() will get back a string of whatever size |str| had on entry
// to this function (probably 0).
template <class string_type>
inline typename string_type::value_type *write_info(string_type *str, size_t length_with_null) {
  str->reserve(length_with_null);
  str->resize(length_with_null - 1);
  return &((*str)[0]);
}

} // namespace

// reg_key ----------------------------------------------------------------------

reg_key::reg_key() = default;

reg_key::reg_key(HKEY key) : key_(key) {}

reg_key::reg_key(HKEY rootkey, const wchar_t *subkey, REGSAM access) {
  if (rootkey) {
    if (access & (KEY_SET_VALUE | KEY_CREATE_SUB_KEY | KEY_CREATE_LINK)) {
      (void)create(rootkey, subkey, access);
    } else {
      (void)open(rootkey, subkey, access);
    }
  } else {
    wow64access_ = access & k_wow64_access_mask;
  }
}

reg_key::reg_key(reg_key &&other) noexcept : key_(other.key_), wow64access_(other.wow64access_) {
  other.key_ = nullptr;
  other.wow64access_ = 0;
}

reg_key &reg_key::operator=(reg_key &&other) {
  close();
  std::swap(key_, other.key_);
  std::swap(wow64access_, other.wow64access_);
  return *this;
}

reg_key::~reg_key() { close(); }

LONG reg_key::create(HKEY rootkey, const wchar_t *subkey, REGSAM access) {
  DWORD disposition_value;
  return create_with_disposition(rootkey, subkey, &disposition_value, access);
}

LONG reg_key::create_with_disposition(HKEY rootkey, const wchar_t *subkey, DWORD *disposition,
                                      REGSAM access) {
  HKEY subhkey = nullptr;
  LONG result = ::RegCreateKeyExW(rootkey, subkey, 0, nullptr, REG_OPTION_NON_VOLATILE, access,
                                  nullptr, &subhkey, disposition);
  if (result == ERROR_SUCCESS) {
    close();
    key_ = subhkey;
    wow64access_ = access & k_wow64_access_mask;
  }

  return result;
}

LONG reg_key::create_key(const wchar_t *name, REGSAM access) {
  if (!valid()) {
    // The parent key has not been opened or created.
    return ERROR_INVALID_HANDLE;
  }

  // After the application has accessed an alternate registry view using one
  // of the [KEY_WOW64_32KEY / KEY_WOW64_64KEY] flags, all subsequent
  // operations (create, delete, or open) on child registry keys must
  // explicitly use the same flag. Otherwise, there can be unexpected
  // behavior.
  // http://msdn.microsoft.com/en-us/library/windows/desktop/aa384129.aspx.
  if ((access & k_wow64_access_mask) != wow64access_) {
    return ERROR_INVALID_PARAMETER;
  }
  HKEY subkey = nullptr;
  LONG result = ::RegCreateKeyExW(key_, name, 0, nullptr, REG_OPTION_NON_VOLATILE, access, nullptr,
                                  &subkey, nullptr);
  if (result == ERROR_SUCCESS) {
    close();
    key_ = subkey;
    wow64access_ = access & k_wow64_access_mask;
  }

  return result;
}

LONG reg_key::open(HKEY rootkey, const wchar_t *subkey, REGSAM access) {
  return open(rootkey, subkey, /*options=*/0, access);
}

LONG reg_key::open_key(const wchar_t *relative_key_name, REGSAM access) {
  if (!valid()) {
    // The parent key has not been opened or created.
    return ERROR_INVALID_HANDLE;
  }

  // After the application has accessed an alternate registry view using one
  // of the [KEY_WOW64_32KEY / KEY_WOW64_64KEY] flags, all subsequent
  // operations (create, delete, or open) on child registry keys must
  // explicitly use the same flag. Otherwise, there can be unexpected
  // behavior.
  // http://msdn.microsoft.com/en-us/library/windows/desktop/aa384129.aspx.
  if ((access & k_wow64_access_mask) != wow64access_) {
    return ERROR_INVALID_PARAMETER;
  }
  HKEY subkey = nullptr;
  LONG result = ::RegOpenKeyExW(key_, relative_key_name, 0, access, &subkey);

  // We have to close the current opened key before replacing it with the new
  // one.
  if (result == ERROR_SUCCESS) {
    close();
    key_ = subkey;
    wow64access_ = access & k_wow64_access_mask;
  }
  return result;
}

void reg_key::close() {
  if (key_) {
    ::RegCloseKey(key_);
    key_ = nullptr;
    wow64access_ = 0;
  }
}

// TODO(wfh): Remove this and other unsafe methods. See http://crbug.com/375400
void reg_key::set(HKEY key) {
  if (key_ != key) {
    close();
    key_ = key;
  }
}

HKEY reg_key::take() {
  HKEY key = key_;
  key_ = nullptr;
  return key;
}

bool reg_key::has_value(const wchar_t *name) const {
  return ::RegQueryValueExW(key_, name, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS;
}

DWORD reg_key::get_value_count() const {
  DWORD count = 0;
  ::RegQueryInfoKeyW(key_, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &count, nullptr,
                     nullptr, nullptr, nullptr);
  return count;
}

LONG reg_key::get_value_name_at(DWORD index, std::wstring *name) const {
  wchar_t buf[256];
  DWORD bufsize = static_cast<DWORD>(std::size(buf));
  LONG r = ::RegEnumValueW(key_, index, buf, &bufsize, nullptr, nullptr, nullptr, nullptr);
  if (r == ERROR_SUCCESS) {
    name->assign(buf, bufsize);
  }

  return r;
}

LONG reg_key::delete_key(const wchar_t *name, bool recursive) {
  if (!valid()) {
    return ERROR_INVALID_HANDLE;
  }

  // Verify the key exists before attempting delete to replicate previous
  // behavior.
  reg_key target_key;
  LONG result =
      target_key.open(key_, name, REG_OPTION_OPEN_LINK, wow64access_ | KEY_QUERY_VALUE | DELETE);
  if (result != ERROR_SUCCESS) {
    return result;
  }

  if (recursive) {
    target_key.close();
    return reg_del_recurse(key_, name, wow64access_);
  }

  // Next, try to delete the key if it is a symbolic link.
  if (auto deleted_link = target_key.delete_if_link(); deleted_link.has_value()) {
    return deleted_link.value();
  }

  // It's not a symbolic link, so try to delete it without recursing.
  return ::RegDeleteKeyExW(target_key.key_, L"", wow64access_, 0);
}

LONG reg_key::delete_value(const wchar_t *value_name) {
  // `RegDeleteValue()` will return an error if `key_` is invalid.
  LONG result = ::RegDeleteValueW(key_, value_name);
  return result;
}

LONG reg_key::read_value_dw(const wchar_t *name, DWORD *out_value) const {
  DWORD type = REG_DWORD;
  DWORD size = sizeof(DWORD);
  DWORD local_value = 0;
  LONG result = read_value(name, &local_value, &size, &type);
  if (result == ERROR_SUCCESS) {
    if ((type == REG_DWORD || type == REG_BINARY) && size == sizeof(DWORD)) {
      *out_value = local_value;
    } else {
      result = ERROR_CANTREAD;
    }
  }

  return result;
}

LONG reg_key::read_value_int64(const wchar_t *name, int64_t *out_value) const {
  DWORD type = REG_QWORD;
  int64_t local_value = 0;
  DWORD size = sizeof(local_value);
  LONG result = read_value(name, &local_value, &size, &type);
  if (result == ERROR_SUCCESS) {
    if ((type == REG_QWORD || type == REG_BINARY) && size == sizeof(local_value)) {
      *out_value = local_value;
    } else {
      result = ERROR_CANTREAD;
    }
  }

  return result;
}

LONG reg_key::read_value(const wchar_t *name, std::wstring *out_value) const {
  const size_t k_max_string_length = 1024; // This is after expansion.
  // Use the one of the other forms of read_value if 1024 is too small for you.
  wchar_t raw_value[k_max_string_length];
  DWORD type = REG_SZ, size = sizeof(raw_value);
  LONG result = read_value(name, raw_value, &size, &type);
  if (result == ERROR_SUCCESS) {
    if (type == REG_SZ) {
      *out_value = raw_value;
    } else if (type == REG_EXPAND_SZ) {
      wchar_t expanded[k_max_string_length];
      size = ExpandEnvironmentStringsW(raw_value, expanded, k_max_string_length);
      // Success: returns the number of wchar_t's copied
      // Fail: buffer too small, returns the size required
      // Fail: other, returns 0
      if (size == 0 || size > k_max_string_length) {
        result = ERROR_MORE_DATA;
      } else {
        *out_value = expanded;
      }
    } else {
      // Not a string. Oops.
      result = ERROR_CANTREAD;
    }
  }

  return result;
}

LONG reg_key::read_value(const wchar_t *name, void *data, DWORD *dsize, DWORD *dtype) const {
  LONG result =
      ::RegQueryValueExW(key_, name, nullptr, dtype, reinterpret_cast<LPBYTE>(data), dsize);
  return result;
}

LONG reg_key::read_values(const wchar_t *name, std::vector<std::wstring> *values) {
  values->clear();

  DWORD type = REG_MULTI_SZ;
  DWORD size = 0;
  LONG result = read_value(name, nullptr, &size, &type);
  if (result != ERROR_SUCCESS || size == 0) {
    return result;
  }

  if (type != REG_MULTI_SZ) {
    return ERROR_CANTREAD;
  }

  std::vector<wchar_t> buffer(size / sizeof(wchar_t));
  result = read_value(name, buffer.data(), &size, nullptr);
  if (result != ERROR_SUCCESS || size == 0) {
    return result;
  }

  // Parse the double-null-terminated list of strings.
  // Note: This code is paranoid to not read outside of |buf|, in the case where
  // it may not be properly terminated.
  auto entry = buffer.cbegin();
  auto buffer_end = buffer.cend();
  while (entry < buffer_end && *entry != '\0') {
    auto entry_end = std::find(entry, buffer_end, '\0');
    values->emplace_back(entry, entry_end);
    entry = entry_end + 1;
  }
  return 0;
}

LONG reg_key::write_value(const wchar_t *name, DWORD in_value) {
  return write_value(name, &in_value, static_cast<DWORD>(sizeof(in_value)), REG_DWORD);
}

LONG reg_key::write_value(const wchar_t *name, const wchar_t *in_value) {
  return write_value(
      name, in_value,
      static_cast<DWORD>(sizeof(*in_value) * (std::char_traits<wchar_t>::length(in_value) + 1)),
      REG_SZ);
}

LONG reg_key::write_value(const wchar_t *name, const void *data, DWORD dsize, DWORD dtype) {
  LONG result = ::RegSetValueExW(key_, name, 0, dtype,
                                 reinterpret_cast<LPBYTE>(const_cast<void *>(data)), dsize);
  return result;
}

LONG reg_key::open(HKEY rootkey, const wchar_t *subkey, DWORD options, REGSAM access) {
  HKEY subhkey = nullptr;

  LONG result = ::RegOpenKeyExW(rootkey, subkey, options, access, &subhkey);
  if (result == ERROR_SUCCESS) {
    close();
    key_ = subhkey;
    wow64access_ = access & k_wow64_access_mask;
  }

  return result;
}

bool reg_key::is_link() const {
  DWORD value_type = 0;
  LONG result = ::RegQueryValueExW(key_, L"SymbolicLinkValue",
                                   /*lpReserved=*/nullptr, &value_type,
                                   /*lpData=*/nullptr, /*lpcbData=*/nullptr);
  if (result == ERROR_FILE_NOT_FOUND) {
    return false;
  }

  if (result == ERROR_SUCCESS) {
    return value_type == REG_LINK;
  }

  return false;
}

std::optional<LONG> reg_key::delete_if_link() {
  if (!is_link()) {
    return std::nullopt;
  };

  NTSTATUS delete_result = STATUS_NOT_FOUND;

  using NtDeleteKey = NTSTATUS(NTAPI *)(HANDLE);
  static const NtDeleteKey nt_delete_key = reinterpret_cast<NtDeleteKey>(
      ::GetProcAddress(::GetModuleHandleW(L"ntdll.dll"), "NtDeleteKey"));
  if (nt_delete_key) {
    delete_result = nt_delete_key(key_);
  }

  if (delete_result == STATUS_SUCCESS) {
    return ERROR_SUCCESS;
  }
  using RtlNtStatusToDosErrorFunction = ULONG(WINAPI *)(NTSTATUS);
  static const RtlNtStatusToDosErrorFunction rtl_nt_status_to_dos_error =
      reinterpret_cast<RtlNtStatusToDosErrorFunction>(
          ::GetProcAddress(::GetModuleHandleW(L"ntdll.dll"), "RtlNtStatusToDosError"));
  // The most common cause of failure is the presence of subkeys, which is
  // reported as `STATUS_CANNOT_DELETE` and maps to `ERROR_ACCESS_DENIED`.
  return rtl_nt_status_to_dos_error ? static_cast<LONG>(rtl_nt_status_to_dos_error(delete_result))
                                    : ERROR_ACCESS_DENIED;
}

// static
LONG reg_key::reg_del_recurse(HKEY root_key, const wchar_t *name, REGSAM access) {
  // First, open the key; taking care not to traverse symbolic links.
  reg_key target_key;
  LONG result = target_key.open(root_key, name, REG_OPTION_OPEN_LINK,
                                access | KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE | DELETE);
  if (result == ERROR_FILE_NOT_FOUND) { // The key doesn't exist.
    return ERROR_SUCCESS;
  }
  if (result != ERROR_SUCCESS) {
    return result;
  }

  // Next, try to delete the key if it is a symbolic link.
  if (auto deleted_link = target_key.delete_if_link(); deleted_link.has_value()) {
    return deleted_link.value();
  }

  // It's not a symbolic link, so try to delete it without recursing.
  result = ::RegDeleteKeyExW(target_key.key_, L"", access, 0);
  if (result == ERROR_SUCCESS) {
    return result;
  }

  // Enumerate the keys.
  const DWORD kMaxKeyNameLength = 256; // Includes string terminator.
  auto subkey_buffer = std::make_unique<wchar_t[]>(kMaxKeyNameLength);
  while (true) {
    DWORD key_size = kMaxKeyNameLength;
    if (::RegEnumKeyExW(target_key.key_, 0, &subkey_buffer[0], &key_size, nullptr, nullptr, nullptr,
                        nullptr) != ERROR_SUCCESS) {
      break;
    }
    if (reg_del_recurse(target_key.key_, &subkey_buffer[0], access) != ERROR_SUCCESS) {
      break;
    }
  }

  // Try again to delete the key.
  return ::RegDeleteKeyExW(target_key.key_, L"", access, 0);
}

// reg_value_iterator ------------------------------------------------------

reg_value_iterator::reg_value_iterator(HKEY root_key, const wchar_t *folder_key, REGSAM wow64access)
    : name_(MAX_PATH, '\0'), value_(MAX_PATH, '\0') {
  initialize(root_key, folder_key, wow64access);
}

reg_value_iterator::reg_value_iterator(HKEY root_key, const wchar_t *folder_key)
    : name_(MAX_PATH, '\0'), value_(MAX_PATH, '\0') {
  initialize(root_key, folder_key, 0);
}

void reg_value_iterator::initialize(HKEY root_key, const wchar_t *folder_key, REGSAM wow64access) {
  LONG result = ::RegOpenKeyExW(root_key, folder_key, 0, KEY_READ | wow64access, &key_);
  if (result != ERROR_SUCCESS) {
    key_ = nullptr;
  } else {
    DWORD count = 0;
    result = ::RegQueryInfoKeyW(key_, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &count,
                                nullptr, nullptr, nullptr, nullptr);

    if (result != ERROR_SUCCESS) {
      ::RegCloseKey(key_);
      key_ = nullptr;
    } else {
      index_ = count - 1;
    }
  }

  read();
}

reg_value_iterator::~reg_value_iterator() {
  if (key_)
    ::RegCloseKey(key_);
}

DWORD reg_value_iterator::value_count() const {
  DWORD count = 0;
  LONG result = ::RegQueryInfoKeyW(key_, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                                   &count, nullptr, nullptr, nullptr, nullptr);
  if (result != ERROR_SUCCESS)
    return 0;

  return count;
}

bool reg_value_iterator::valid() const { return key_ != nullptr && index_ != k_invalid_iter_value; }

void reg_value_iterator::operator++() {
  if (index_ != k_invalid_iter_value)
    --index_;
  read();
}

bool reg_value_iterator::read() {
  if (valid()) {
    DWORD capacity = static_cast<DWORD>(name_.capacity());
    DWORD name_size = capacity;
    // |value_size_| is in bytes. Reserve the last character for a NUL.
    value_size_ = static_cast<DWORD>((value_.size() - 1) * sizeof(wchar_t));
    LONG result = ::RegEnumValueW(key_, index_, write_info(&name_, name_size), &name_size, nullptr,
                                  &type_, reinterpret_cast<BYTE *>(value_.data()), &value_size_);

    if (result == ERROR_MORE_DATA) {
      // Registry key names are limited to 255 characters and fit within
      // MAX_PATH (which is 260) but registry value names can use up to 16,383
      // characters and the value itself is not limited
      // (from http://msdn.microsoft.com/en-us/library/windows/desktop/
      // ms724872(v=vs.85).aspx).
      // Resize the buffers and retry if their size caused the failure.
      DWORD value_size_in_wchars = to_wchar_size(value_size_);
      if (value_size_in_wchars + 1 > value_.size())
        value_.resize(value_size_in_wchars + 1, '\0');
      value_size_ = static_cast<DWORD>((value_.size() - 1) * sizeof(wchar_t));
      name_size = name_size == capacity ? k_max_registry_name_size : capacity;
      result = ::RegEnumValueW(key_, index_, write_info(&name_, name_size), &name_size, nullptr,
                               &type_, reinterpret_cast<BYTE *>(value_.data()), &value_size_);
    }

    if (result == ERROR_SUCCESS) {
      value_[to_wchar_size(value_size_)] = '\0';
      return true;
    }
  }

  name_[0] = '\0';
  value_[0] = '\0';
  value_size_ = 0;
  return false;
}

// reg_key_iterator --------------------------------------------------------

reg_key_iterator::reg_key_iterator(HKEY root_key, const wchar_t *folder_key) {
  initialize(root_key, folder_key, 0);
}

reg_key_iterator::reg_key_iterator(HKEY root_key, const wchar_t *folder_key, REGSAM wow64access) {
  initialize(root_key, folder_key, wow64access);
}

reg_key_iterator::~reg_key_iterator() {
  if (key_)
    ::RegCloseKey(key_);
}

DWORD reg_key_iterator::sub_key_count() const {
  DWORD count = 0;
  LONG result = ::RegQueryInfoKeyW(key_, nullptr, nullptr, nullptr, &count, nullptr, nullptr,
                                   nullptr, nullptr, nullptr, nullptr, nullptr);
  if (result != ERROR_SUCCESS)
    return 0;

  return count;
}

bool reg_key_iterator::valid() const { return key_ != nullptr && index_ != k_invalid_iter_value; }

void reg_key_iterator::operator++() {
  if (index_ != k_invalid_iter_value)
    --index_;
  read();
}

bool reg_key_iterator::read() {
  if (valid()) {
    DWORD ncount = static_cast<DWORD>(std::size(name_));
    FILETIME written;
    LONG r = ::RegEnumKeyExW(key_, index_, name_, &ncount, nullptr, nullptr, nullptr, &written);
    if (ERROR_SUCCESS == r)
      return true;
  }

  name_[0] = '\0';
  return false;
}

void reg_key_iterator::initialize(HKEY root_key, const wchar_t *folder_key, REGSAM wow64access) {
  LONG result = ::RegOpenKeyExW(root_key, folder_key, 0, KEY_READ | wow64access, &key_);
  if (result != ERROR_SUCCESS) {
    key_ = nullptr;
  } else {
    DWORD count = 0;
    result = ::RegQueryInfoKeyW(key_, nullptr, nullptr, nullptr, &count, nullptr, nullptr, nullptr,
                                nullptr, nullptr, nullptr, nullptr);

    if (result != ERROR_SUCCESS) {
      ::RegCloseKey(key_);
      key_ = nullptr;
    } else {
      index_ = count - 1;
    }
  }

  read();
}

} // namespace base
} // namespace traa