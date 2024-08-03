#include "base/folder/folder.h"
#include "base/platform.h"

namespace traa {
namespace base {

namespace {

#if defined(TRAA_OS_WINDOWS)
const char kEndChar = '\0';
const char kFilePathSeparators[] = "\\/";
const char kFilePathCurrentDirectory[] = ".";
const char kFilePathParentDirectory[] = "..";
const char kFilePathExtensionSeparator = '.';
#else
const char kEndChar = '\0';
const char kFilePathSeparators[] = "/";
const char kFilePathCurrentDirectory[] = ".";
const char kFilePathParentDirectory[] = "..";
const char kFilePathExtensionSeparator = '.';
#endif // TRAA_OS_WINDOWS

static bool is_separator(const char separator) {
  if (separator == kEndChar) {
    return false;
  }

  for (size_t i = 0; i < sizeof(kFilePathSeparators); ++i) {
    if (separator == kFilePathSeparators[i]) {
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
    size_t separators_count = sizeof(kFilePathSeparators) / sizeof(char);
    for (size_t index = 0; index < separators_count; index++) {
      separator_pos = path.rfind(kFilePathSeparators[index]);
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
    size_t pos = file_name.rfind(kFilePathExtensionSeparator);
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
  return *path.rbegin() == kFilePathSeparators[0] || *path.rbegin() == kFilePathSeparators[1];
#else
  return *path.rbegin() == kFilePathSeparators[0];
#endif // OS_WIN
}

void folder::append_filename(std::string &path, const char *filename) {
  if (path.size() == 0) {
    path = filename;
    return;
  }

  if (filename == nullptr || filename[0] == kEndChar) {
    return;
  }

  if (is_separator(path[path.size() - 1])) {
    path += filename;
  } else {
    path += kFilePathSeparators[0];
    path += filename;
  }
}

void folder::append_filename(std::string &path, const std::string &filename) {
  append_filename(path, filename.c_str());
}

} // namespace base
} // namespace traa