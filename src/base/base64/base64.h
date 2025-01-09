
//*********************************************************************
//* C_Base64 - a simple base64 encoder and decoder.
//*
//*     Copyright (c) 1999, Bob Withers - bwit@pobox.com
//*
//* This code may be freely used for any purpose, either personal
//* or commercial, provided the authors copyright notice remains
//* intact.
//*********************************************************************

#ifndef TRAA_BASE_BASE64_BASE64_H_
#define TRAA_BASE_BASE64_BASE64_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace traa {
namespace base {

class base64 {
public:
  enum decode_option {
    DO_PARSE_STRICT = 1, // Parse only base64 characters
    DO_PARSE_WHITE = 2,  // Parse only base64 and whitespace characters
    DO_PARSE_ANY = 3,    // Parse all characters
    DO_PARSE_MASK = 3,

    DO_PAD_YES = 4, // Padding is required
    DO_PAD_ANY = 8, // Padding is optional
    DO_PAD_NO = 12, // Padding is disallowed
    DO_PAD_MASK = 12,

    DO_TERM_BUFFER = 16, // Must termiante at end of buffer
    DO_TERM_CHAR = 32,   // May terminate at any character boundary
    DO_TERM_ANY = 48,    // May terminate at a sub-character bit offset
    DO_TERM_MASK = 48,

    // Strictest interpretation
    DO_STRICT = DO_PARSE_STRICT | DO_PAD_YES | DO_TERM_BUFFER,

    DO_LAX = DO_PARSE_ANY | DO_PAD_ANY | DO_TERM_CHAR,
  };
  typedef int decode_flags;

  static bool is_base64_char(char ch);

  // Get the char next to the `ch` from the base64_table.
  // If the `ch` is the last one in the base64_table then returns
  // the first one from the table.
  // Expects the `ch` be a base64 char.
  // The result will be saved in `next_ch`.
  // Returns true on success.
  static bool get_next_base64_char(char ch, char *next_ch);

  // Determines whether the given string consists entirely of valid base64
  // encoded characters.
  static bool is_base64_encoded(std::string_view str);

  static void encode_from_array(const void *data, size_t len, std::string *result);
  static bool decode_from_array(const char *data, size_t len, decode_flags flags,
                                std::string *result, size_t *data_used);
  static bool decode_from_array(const char *data, size_t len, decode_flags flags,
                                std::vector<char> *result, size_t *data_used);
  static bool decode_from_array(const char *data, size_t len, decode_flags flags,
                                std::vector<uint8_t> *result, size_t *data_used);

  // Convenience Methods
  static inline std::string Encode(std::string_view data) {
    std::string result;
    encode_from_array(data.data(), data.size(), &result);
    return result;
  }
  static inline std::string decode(std::string_view data, decode_flags flags) {
    std::string result;
    decode_from_array(data.data(), data.size(), flags, &result, nullptr);
    return result;
  }
  static inline bool decode(std::string_view data, decode_flags flags, std::string *result,
                            size_t *data_used) {
    return decode_from_array(data.data(), data.size(), flags, result, data_used);
  }
  static inline bool decode(std::string_view data, decode_flags flags, std::vector<char> *result,
                            size_t *data_used) {
    return decode_from_array(data.data(), data.size(), flags, result, data_used);
  }

private:
  static const char base64_table[];
  static const unsigned char decode_table[];

  static size_t get_next_quantum(decode_flags parse_flags, bool illegal_pads, const char *data,
                                 size_t len, size_t *dpos, unsigned char qbuf[4], bool *padded);
  template <typename T>
  static bool decode_from_array_template(const char *data, size_t len, decode_flags flags,
                                         T *result, size_t *data_used);
};

} // namespace base
} // namespace traa

#endif /* TRAA_BASE_BASE64_BASE64_H_ */
