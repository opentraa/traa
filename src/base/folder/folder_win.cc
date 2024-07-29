#include "base/folder/folder.h"
#include "base/strings/string_trans.h"

#include <Windows.h>
#include <shlobj.h>

namespace traa {
namespace base {

std::string folder::get_current_folder() {
  wchar_t buffer[MAX_PATH];
  GetModuleFileNameW(::GetModuleHandle(NULL), buffer, MAX_PATH);
  std::wstring::size_type pos = std::wstring(buffer).find_last_of(L"\\/");
  return string_trans::unicode_to_utf8(std::wstring(buffer).substr(0, pos));
}

std::string folder::get_config_folder() {
  wchar_t buffer[MAX_PATH];
  SHGetSpecialFolderPathW(NULL, buffer, CSIDL_LOCAL_APPDATA, FALSE);
  return string_trans::unicode_to_utf8(std::wstring(buffer));
}

std::string folder::get_temp_folder() {
  wchar_t buffer[MAX_PATH];
  GetTempPathW(MAX_PATH, buffer);
  return string_trans::unicode_to_utf8(std::wstring(buffer));
}

bool folder::create_folder(const std::string &folder) {
  if (folder.empty()) {
    return false;
  }

  std::wstring wfolder = string_trans::utf8_to_unicode(folder);
  return CreateDirectoryW(wfolder.c_str(), NULL) || ERROR_ALREADY_EXISTS == GetLastError();
}

} // namespace base
} // namespace traa