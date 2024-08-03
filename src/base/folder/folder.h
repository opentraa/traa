#ifndef TRAA_BASE_FOLDER_FOLDER_H
#define TRAA_BASE_FOLDER_FOLDER_H

#include "base/disallow.h"

#include <string>

namespace traa {
namespace base {

/**
 * @class folder
 * @brief Utility class for working with folders and paths.
 *
 * The folder class provides various static methods for manipulating folders and paths.
 * It includes methods for getting the filename, directory, and file extension from a path,
 * checking if a path is a directory, appending a filename to a path, getting the current folder
 * of the executable file, getting the config folder of the current user, getting the temp folder
 * of the current user, and creating a folder.
 */
class folder {
  DISALLOW_IMPLICIT_CONSTRUCTORS(folder);

public:
  /**
   * Get the filename from a path.
   * @param path The path to get the filename from.
   * @return The filename.
   */
  static std::string get_filename(const std::string &path);

  /**
   * Get the directory from a path.
   * @param path The path to get the directory from.
   * @return The directory.
   */
  static std::string get_directory(const std::string &path);

  /**
   * Get the file extension from a path.
   * @param path The path to get the file extension from.
   * @return The file extension.
   */
  static std::string get_file_extension(const std::string &path);

  /**
   * Check if a path is a directory or not.
   * @param path The path to determine if it is a directory or not.
   * @return True if the path is a directory, false otherwise.
   */
  static bool is_directory(const std::string &path);

  /**
   * Append a filename to a path, this will append the filename to the path with the correct
   * separator.
   * @param path The path to append the filename to.
   * @param filename The filename to append to the path.
   */
  static void append_filename(std::string &path, const char *filename);

  /**
   * Append a filename to a path, this will append the filename to the path with the correct
   * separator.
   * @param path The path to append the filename to.
   * @param filename The filename to append to the path.
   */
  static void append_filename(std::string &path, const std::string &filename);

  /**
   * Get the current folder of the executable file.
   * @return The current folder of the executable file.
   */
  static std::string get_current_folder();

  /**
   * Get the config folder of the current user.
   * @return The config folder of the current user.
   */
  static std::string get_config_folder();

  /**
   * Get the temp folder of the current user.
   * @return The temp folder of the current user.
   */
  static std::string get_temp_folder();

  /**
   * Create a folder.
   * @param folder The folder to create.
   * @return True if the folder is created successfully, false otherwise.
   */
  static bool create_folder(const std::string &folder);
};
} // namespace base
} // namespace traa

#endif // TRAA_BASE_FOLDER_FOLDER_H