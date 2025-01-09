#ifndef TRAA_BASE_DEVICES_SCREEN_TEST_SIMPLE_WINDOW_SIMPLE_WINDOW_H_
#define TRAA_BASE_DEVICES_SCREEN_TEST_SIMPLE_WINDOW_SIMPLE_WINDOW_H_

#include "base/platform.h"

#include <memory>
#include <string>

namespace traa {
namespace base {

class simple_window {
public:
  // Define an arbitrary color for the test window with unique R, G, and B values
  // so consumers can verify captured content in tests.
  static constexpr uint8_t k_r_value = 191;
  static constexpr uint8_t k_g_value = 99;
  static constexpr uint8_t k_b_value = 12;

  static constexpr uint8_t k_default_width = 200;
  static constexpr uint8_t k_default_height = 200;

public:
  static std::shared_ptr<simple_window> create(const std::string &title,
                                               uint32_t width = k_default_width,
                                               uint32_t height = k_default_height,
                                               uint64_t style = 0);

  virtual ~simple_window() = default;

  virtual std::string get_title() const { return title_; }

  virtual intptr_t get_source_id() const { return source_id_; }

  virtual intptr_t get_view() const { return view_id_; }

  virtual void minimized() {}

  virtual void unminimized() {}

  virtual void resize(uint32_t width, uint32_t height) {}

  virtual void move(uint32_t x, uint32_t y) {}

protected:
  std::string title_;
  intptr_t source_id_;
  intptr_t view_id_;
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_TEST_SIMPLE_WINDOW_SIMPLE_WINDOW_H_
