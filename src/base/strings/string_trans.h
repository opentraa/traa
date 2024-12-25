#ifndef TRAA_BASE_STRINGS_STRING_TRANS_H_
#define TRAA_BASE_STRINGS_STRING_TRANS_H_

#include "base/disallow.h"

#include <string>

namespace traa {
namespace base {

// TODO @sylar: optimize this to use string_view

/**
 * @class string_trans
 * @brief Utility class for string transformations.
 *
 * This class provides static methods to convert strings between different encodings.
 */
class string_trans {
  DISALLOW_IMPLICIT_CONSTRUCTORS(string_trans);

public:
  /**
   * Convert an ASCII string to a Unicode string.
   * @param str The ASCII string to be converted.
   * @return The Unicode string.
   */
  static std::wstring ascii_to_unicode(const std::string &str);

  /**
   * Convert a Unicode string to an ASCII string.
   * @param wstr The Unicode string to be converted.
   * @return The ASCII string.
   */
  static std::string unicode_to_ascii(const std::wstring &wstr);

  /**
   * Convert an ASCII string to a UTF-8 string.
   * @param str The ASCII string to be converted.
   * @return The UTF-8 string.
   */
  static std::string ascii_to_utf8(const std::string &str);

  /**
   * Convert a UTF-8 string to an ASCII string.
   * @param utf8 The UTF-8 string to be converted.
   * @return The ASCII string.
   */
  static std::string utf8_to_ascii(const std::string &utf8);

  /**
   * Convert a Unicode string to a UTF-8 string.
   * @param wstr The Unicode string to be converted.
   * @return The UTF-8 string.
   */
  static std::string unicode_to_utf8(const std::wstring &wstr);

  /**
   * Convert a UTF-8 string to a Unicode string.
   * @param utf8 The UTF-8 string to be converted.
   * @return The Unicode string.
   */
  static std::wstring utf8_to_unicode(const std::string &utf8);
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_STRINGS_STRING_TRANS_H_