#include "base/folder/folder.h"
#include "base/platform.h"

namespace traa {
namespace base {

namespace {

#if defined(TRAA_OS_WINDOWS)
const char k_end_cahr = '\0';
const char k_file_path_separators[] = "\\/";
const char k_file_path_cur_directory[] = ".";
const char k_file_path_parent_directory[] = "..";
const char k_file_path_ext_separator = '.';
#else
const char k_end_cahr = '\0';
const char k_file_path_separators[] = "/";
const char k_file_path_cur_directory[] = ".";
const char k_file_path_parent_directory[] = "..";
const char k_file_path_ext_separator = '.';
#endif // TRAA_OS_WINDOWS

static bool is_separator(const char separator) {
  if (separator == k_end_cahr) {
    return false;
  }

  for (size_t i = 0; i < sizeof(k_file_path_separators); ++i) {
    if (separator == k_file_path_separators[i]) {
      return true;
    }
  }
  return false;
}

static bool is_separator(const std::string &separator) {
  if (separator.empty()) {
    return false;
  }

  return is_separator(separator[0]);
}
} // namespace

std::string folder::get_filename(const std::string &path) {
  if (path.size() == 0) {
    return std::string();
  }

  std::string filename;
  do {
    size_t separator_pos = std::string::npos;
    size_t separators_count = sizeof(k_file_path_separators) / sizeof(char);
    for (size_t index = 0; index < separators_count; index++) {
      separator_pos = path.rfind(k_file_path_separators[index]);
      if (separator_pos != std::string::npos) {
        break;
      }
    }

    if (separator_pos++ != std::string::npos && separator_pos < path.size()) {
      filename = path.substr(separator_pos);
    } else if (separator_pos >= path.size()) {
      break;
    } else {
      filename = path;
    }
  } while (0);

  return filename;
}

std::string folder::get_directory(const std::string &path) {
  size_t index = path.size() - 1;
  if (index <= 0 || path.size() == 0)
    return std::string();

  std::string directory;
  for (; index != 0; index--) {
    if (is_separator(path[index])) {
      if (index == path.size() - 1) {
        directory = path;
      } else {
        directory = path.substr(0, index + 1);
      }
      break;
    }
  }

  return directory;
}

std::string folder::get_file_extension(const std::string &path) {
  if (path.size() == 0) {
    return std::string();
  }

  std::string extension;
  std::string file_name = get_filename(path);
  if (file_name.size() > 0) {
    size_t pos = file_name.rfind(k_file_path_ext_separator);
    if (pos != std::string::npos) {
      extension = file_name.substr(pos, std::string::npos);
    }
  }

  return extension;
}

bool folder::is_directory(const std::string &path) {
  if (path.empty())
    return false;
#if defined(TRAA_OS_WINDOWS)
  return *path.rbegin() == k_file_path_separators[0] || *path.rbegin() == k_file_path_separators[1];
#else
  return *path.rbegin() == k_file_path_separators[0];
#endif // OS_WIN
}

void folder::append_filename(std::string &path, const char *filename) {
  if (path.size() == 0) {
    path = filename;
    return;
  }

  if (filename == nullptr || filename[0] == k_end_cahr) {
    return;
  }

  if (is_separator(path[path.size() - 1])) {
    path += filename;
  } else {
    path += k_file_path_separators[0];
    path += filename;
  }
}

void folder::append_filename(std::string &path, const std::string &filename) {
  append_filename(path, filename.c_str());
}

} // namespace base
} // namespace traa