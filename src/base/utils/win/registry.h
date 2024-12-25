#ifndef TRAA_BASE_UTILS_WIN_REGISTRY_H_
#define TRAA_BASE_UTILS_WIN_REGISTRY_H_

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <windows.h>

namespace traa {
namespace base {

// Utility class to read, write and manipulate the Windows Registry.
// Registry vocabulary primer: a "key" is like a folder, in which there
// are "values", which are <name, data> pairs, with an associated data type.
//
// Note:
//  * read_value family of functions guarantee that the out-parameter
//    is not touched in case of failure.
//  * Functions returning LONG indicate success as ERROR_SUCCESS or an
//    error as a (non-zero) win32 error code.
class reg_key {
public:
  reg_key();
  explicit reg_key(HKEY key);
  reg_key(HKEY rootkey, const wchar_t *subkey, REGSAM access);
  reg_key(reg_key &&other) noexcept;
  reg_key &operator=(reg_key &&other);

  reg_key(const reg_key &) = delete;
  reg_key &operator=(const reg_key &) = delete;

  ~reg_key();

  // Creates a new reg key, replacing `this` with a reference to the
  // newly-opened key. In case of error, `this` is unchanged.
  [[nodiscard]] LONG create(HKEY rootkey, const wchar_t *subkey, REGSAM access);

  // Creates a new reg key, replacing `this` with a reference to the
  // newly-opened key. In case of error, `this` is unchanged.
  [[nodiscard]] LONG create_with_disposition(HKEY rootkey, const wchar_t *subkey,
                                             DWORD *disposition, REGSAM access);

  // Creates a subkey or opens it if it already exists. In case of error, `this`
  // is unchanged.
  [[nodiscard]] LONG create_key(const wchar_t *name, REGSAM access);

  // Opens an existing reg key, replacing `this` with a reference to the
  // newly-opened key. In case of error, `this` is unchanged.
  [[nodiscard]] LONG open(HKEY rootkey, const wchar_t *subkey, REGSAM access);

  // Opens an existing reg key, given the relative key name.
  [[nodiscard]] LONG open_key(const wchar_t *relative_key_name, REGSAM access);

  // Closes this reg key.
  void close();

  // Replaces the handle of the registry key and takes ownership of the handle.
  void set(HKEY key);

  // Transfers ownership away from this object.
  HKEY take();

  // Returns false if this key does not have the specified value, or if an error
  // occurrs while attempting to access it.
  bool has_value(const wchar_t *value_name) const;

  // Returns the number of values for this key, or an error code if the number
  // cannot be determined.
  DWORD get_value_count() const;

  // Determines the nth value's name.
  LONG get_value_name_at(DWORD index, std::wstring *name) const;

  // True while the key is valid.
  bool valid() const { return key_ != nullptr; }

  // Kills a key and, by default, everything that lives below it; please be
  // careful when using it. `recursive` = false may be used to prevent
  // recursion, in which case the key is only deleted if it has no subkeys.
  LONG delete_key(const wchar_t *name, bool recursive = true);

  // Deletes a single value within the key.
  LONG delete_value(const wchar_t *name);

  // Getters:

  // Reads a REG_DWORD (uint32_t) into |out_value|. If |name| is null or empty,
  // reads the key's default value, if any.
  LONG read_value_dw(const wchar_t *name, DWORD *out_value) const;

  // Reads a REG_QWORD (int64_t) into |out_value|. If |name| is null or empty,
  // reads the key's default value, if any.
  LONG read_value_int64(const wchar_t *name, int64_t *out_value) const;

  // Reads a string into |out_value|. If |name| is null or empty, reads
  // the key's default value, if any.
  LONG read_value(const wchar_t *name, std::wstring *out_value) const;

  // Reads a REG_MULTI_SZ registry field into a vector of strings. Clears
  // |values| initially and adds further strings to the list. Returns
  // ERROR_CANTREAD if type is not REG_MULTI_SZ.
  LONG read_values(const wchar_t *name, std::vector<std::wstring> *values);

  // Reads raw data into |data|. If |name| is null or empty, reads the key's
  // default value, if any.
  LONG read_value(const wchar_t *name, void *data, DWORD *dsize, DWORD *dtype) const;

  // Setters:

  // Sets an int32_t value.
  LONG write_value(const wchar_t *name, DWORD in_value);

  // Sets a string value.
  LONG write_value(const wchar_t *name, const wchar_t *in_value);

  // Sets raw data, including type.
  LONG write_value(const wchar_t *name, const void *data, DWORD dsize, DWORD dtype);

  HKEY handle() const { return key_; }

private:
  // Opens the key `subkey` under `rootkey` with the given options and
  // access rights. `options` may be 0 or `REG_OPTION_OPEN_LINK`. Returns
  // ERROR_SUCCESS or a Windows error code.
  [[nodiscard]] LONG open(HKEY rootkey, const wchar_t *subkey, DWORD options, REGSAM access);

  // Returns true if the key is a symbolic link, false if it is not, or a
  // Windows error code in case of a failure to determine. `this` *MUST* have
  // been opened via at least `open(..., REG_OPTION_OPEN_LINK,
  // REG_QUERY_VALUE);`.
  bool is_link() const;

  // Deletes the key if it is a symbolic link. Returns ERROR_SUCCESS if the key
  // was a link and was deleted, a Windows error code if checking the key or
  // deleting it failed, or `nullopt` if the key exists and is not a symbolic
  // link.
  std::optional<LONG> delete_if_link();

  // Recursively deletes a key and all of its subkeys.
  static LONG reg_del_recurse(HKEY root_key, const wchar_t *name, REGSAM access);

  HKEY key_ = nullptr; // The registry key being iterated.
  REGSAM wow64access_ = 0;
};

// Iterates the entries found in a particular folder on the registry.
class reg_value_iterator {
public:
  // Constructs a Registry Value Iterator with default WOW64 access.
  reg_value_iterator(HKEY root_key, const wchar_t *folder_key);

  // Constructs a Registry Key Iterator with specific WOW64 access, one of
  // KEY_WOW64_32KEY or KEY_WOW64_64KEY, or 0.
  // Note: |wow64access| should be the same access used to open |root_key|
  // previously, or a predefined key (e.g. HKEY_LOCAL_MACHINE).
  // See http://msdn.microsoft.com/en-us/library/windows/desktop/aa384129.aspx.
  reg_value_iterator(HKEY root_key, const wchar_t *folder_key, REGSAM wow64access);

  reg_value_iterator(const reg_value_iterator &) = delete;
  reg_value_iterator &operator=(const reg_value_iterator &) = delete;

  ~reg_value_iterator();

  DWORD value_count() const;

  // True while the iterator is valid.
  bool valid() const;

  // Advances to the next registry entry.
  void operator++();

  const wchar_t *name() const { return name_.c_str(); }
  const wchar_t *value() const { return value_.data(); }
  // value_size() is in bytes.
  DWORD value_size() const { return value_size_; }
  DWORD type() const { return type_; }

  DWORD index() const { return index_; }

private:
  // Reads in the current values.
  bool read();

  void initialize(HKEY root_key, const wchar_t *folder_key, REGSAM wow64access);

  // The registry key being iterated.
  HKEY key_;

  // Current index of the iteration.
  DWORD index_;

  // Current values.
  std::wstring name_;
  std::vector<wchar_t> value_;
  DWORD value_size_;
  DWORD type_;
};

class reg_key_iterator {
public:
  // Constructs a Registry Key Iterator with default WOW64 access.
  reg_key_iterator(HKEY root_key, const wchar_t *folder_key);

  // Constructs a Registry Value Iterator with specific WOW64 access, one of
  // KEY_WOW64_32KEY or KEY_WOW64_64KEY, or 0.
  // Note: |wow64access| should be the same access used to open |root_key|
  // previously, or a predefined key (e.g. HKEY_LOCAL_MACHINE).
  // See http://msdn.microsoft.com/en-us/library/windows/desktop/aa384129.aspx.
  reg_key_iterator(HKEY root_key, const wchar_t *folder_key, REGSAM wow64access);

  reg_key_iterator(const reg_key_iterator &) = delete;
  reg_key_iterator &operator=(const reg_key_iterator &) = delete;

  ~reg_key_iterator();

  DWORD sub_key_count() const;

  // True while the iterator is valid.
  bool valid() const;

  // Advances to the next entry in the folder.
  void operator++();

  const wchar_t *name() const { return name_; }

  DWORD index() const { return index_; }

private:
  // Reads in the current values.
  bool read();

  void initialize(HKEY root_key, const wchar_t *folder_key, REGSAM wow64access);

  // The registry key being iterated.
  HKEY key_;

  // Current index of the iteration.
  DWORD index_;

  wchar_t name_[MAX_PATH];
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_UTILS_WIN_REGISTRY_H_