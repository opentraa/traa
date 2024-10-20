#include "base/folder/folder.h"

namespace traa {
namespace base {

std::string folder::get_current_folder() { return std::string(); }

std::string folder::get_config_folder() { return std::string(); }

std::string folder::get_temp_folder() { return std::string(); }

bool folder::create_folder(const std::string &folder) { return false; }

} // namespace base
} // namespace traa