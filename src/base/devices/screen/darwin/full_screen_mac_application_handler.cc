/*
 *  Copyright (c) 2019 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/darwin/full_screen_mac_application_handler.h"
#include "base/devices/screen/darwin/desktop_configuration.h"
#include "base/devices/screen/darwin/window_list_utils.h"
#include "base/function_view.h"

#include <libproc.h>

#include <algorithm>
#include <functional>
#include <string>
#include <string_view>

namespace traa {
namespace base {

inline namespace {

static constexpr const char *k_power_point_slide_show_titles[] = {
    "PowerPoint-Bildschirmpräsentation",
    "Προβολή παρουσίασης PowerPoint",
    "PowerPoint スライド ショー",
    "PowerPoint Slide Show",
    "PowerPoint 幻灯片放映",
    "Presentación de PowerPoint",
    "PowerPoint-slideshow",
    "Presentazione di PowerPoint",
    "Prezentácia programu PowerPoint",
    "Apresentação do PowerPoint",
    "PowerPoint-bildspel",
    "Prezentace v aplikaci PowerPoint",
    "PowerPoint 슬라이드 쇼",
    "PowerPoint-lysbildefremvisning",
    "PowerPoint-vetítés",
    "PowerPoint Slayt Gösterisi",
    "Pokaz slajdów programu PowerPoint",
    "PowerPoint 投影片放映",
    "Демонстрация PowerPoint",
    "Diaporama PowerPoint",
    "PowerPoint-diaesitys",
    "Peragaan Slide PowerPoint",
    "PowerPoint-diavoorstelling",
    "การนำเสนอสไลด์ PowerPoint",
    "Apresentação de slides do PowerPoint",
    "הצגת שקופיות של PowerPoint",
    "عرض شرائح في PowerPoint"};

class full_screen_mac_application_handler : public full_screen_app_handler {
public:
  using title_predicate_t = std::function<bool(std::string_view, std::string_view)>;

  full_screen_mac_application_handler(desktop_capturer::source_id_t source_id,
                                      title_predicate_t title_predicate,
                                      bool ignore_original_window)
      : full_screen_app_handler(source_id), title_predicate_(title_predicate),
        owner_pid_(get_window_owner_pid(static_cast<CGWindowID>(source_id))),
        ignore_original_window_(ignore_original_window) {}

protected:
  using cache_predicate_t = function_view<bool(const desktop_capturer::source_t &)>;

  void invalidate_cache_if_needed(const desktop_capturer::source_list_t &source_list_t,
                                  int64_t timestamp, cache_predicate_t predicate) const {
    if (timestamp != cache_timestamp_) {
      cache_sources_.clear();
      std::copy_if(source_list_t.begin(), source_list_t.end(), std::back_inserter(cache_sources_),
                   predicate);
      cache_timestamp_ = timestamp;
    }
  }

  win_id_t
  find_full_screen_window_with_same_pid(const desktop_capturer::source_list_t &source_list_t,
                                        int64_t timestamp) const {
    invalidate_cache_if_needed(
        source_list_t, timestamp, [&](const desktop_capturer::source_t &src) {
          return src.id != get_source_id() &&
                 get_window_owner_pid(static_cast<CGWindowID>(src.id)) == owner_pid_;
        });
    if (cache_sources_.empty())
      return kCGNullWindowID;

    const auto original_window = get_source_id();
    const std::string title = get_window_title(static_cast<CGWindowID>(original_window));

    // We can ignore any windows with empty titles cause regardless type of
    // application it's impossible to verify that full screen window and
    // original window are related to the same document.
    if (title.empty())
      return kCGNullWindowID;

    desktop_configuration desktop_config =
        desktop_configuration::current(desktop_configuration::COORDINATE_TOP_LEFT);

    const auto it = std::find_if(
        cache_sources_.begin(), cache_sources_.end(), [&](const desktop_capturer::source_t &src) {
          const std::string window_title = get_window_title(static_cast<CGWindowID>(src.id));

          if (window_title.empty())
            return false;

          if (title_predicate_ && !title_predicate_(title, window_title))
            return false;

          return is_window_full_screen(desktop_config, static_cast<CGWindowID>(src.id));
        });

    return it != cache_sources_.end() ? it->id : 0;
  }

  desktop_capturer::source_id_t
  find_full_screen_window(const desktop_capturer::source_list_t &source_list_t,
                          int64_t timestamp) const override {
    return !ignore_original_window_ && is_window_on_screen(static_cast<CGWindowID>(get_source_id()))
               ? 0
               : find_full_screen_window_with_same_pid(source_list_t, timestamp);
  }

protected:
  const title_predicate_t title_predicate_;
  const int owner_pid_;
  const bool ignore_original_window_;
  mutable int64_t cache_timestamp_ = 0;
  mutable desktop_capturer::source_list_t cache_sources_;
};

bool equal_title_predicate(std::string_view original_title, std::string_view title) {
  return original_title == title;
}

bool slide_show_title_predicate(std::string_view original_title, std::string_view title) {
  if (title.find(original_title) == std::string_view::npos)
    return false;

  auto is_starts_with = [](std::string_view text, std::string_view prefix) {
    return prefix.empty() ||
           (text.size() >= prefix.size() && memcmp(text.data(), prefix.data(), prefix.size()) == 0);
  };

  for (const char *pp_slide_title : k_power_point_slide_show_titles) {
    if (is_starts_with(title, pp_slide_title))
      return true;
  }
  return false;
}

class open_office_application_handler : public full_screen_mac_application_handler {
public:
  open_office_application_handler(desktop_capturer::source_id_t source_id)
      : full_screen_mac_application_handler(source_id, nullptr, false) {}

  desktop_capturer::source_id_t
  find_full_screen_window(const desktop_capturer::source_list_t &source_list_t,
                          int64_t timestamp) const override {
    invalidate_cache_if_needed(
        source_list_t, timestamp, [&](const desktop_capturer::source_t &src) {
          return get_window_owner_pid(static_cast<CGWindowID>(src.id)) == owner_pid_;
        });

    const auto original_window = get_source_id();
    const std::string original_title = get_window_title(static_cast<CGWindowID>(original_window));

    // Check if we have only one document window, otherwise it's not possible
    // to securely match a document window and a slide show window which has
    // empty title.
    if (std::any_of(cache_sources_.begin(), cache_sources_.end(),
                    [&original_title](const desktop_capturer::source_t &src) {
                      return src.title.length() && src.title != original_title;
                    })) {
      return kCGNullWindowID;
    }

    desktop_configuration desktop_config =
        desktop_configuration::current(desktop_configuration::COORDINATE_TOP_LEFT);

    // Looking for slide show window,
    // it must be a full screen window with empty title
    const auto slide_show_window =
        std::find_if(cache_sources_.begin(), cache_sources_.end(), [&](const auto &src) {
          return src.title.empty() &&
                 is_window_full_screen(desktop_config, static_cast<CGWindowID>(src.id));
        });

    if (slide_show_window == cache_sources_.end()) {
      return kCGNullWindowID;
    }

    return slide_show_window->id;
  }
};

} // namespace

std::unique_ptr<full_screen_app_handler>
create_full_screen_app_handler(desktop_capturer::source_id_t source_id) {
  std::unique_ptr<full_screen_app_handler> result;
  int pid = get_window_owner_pid(static_cast<CGWindowID>(source_id));
  char buffer[PROC_PIDPATHINFO_MAXSIZE];
  int path_length = proc_pidpath(pid, buffer, sizeof(buffer));
  if (path_length > 0) {
    const char *last_slash = strrchr(buffer, '/');
    const std::string name{last_slash ? last_slash + 1 : buffer};
    const std::string owner_name = get_window_owner_name(static_cast<CGWindowID>(source_id));
    full_screen_mac_application_handler::title_predicate_t predicate = nullptr;
    bool ignore_original_window = false;
    if (name.find("Google Chrome") == 0 || name == "Chromium") {
      predicate = equal_title_predicate;
    } else if (name == "Microsoft PowerPoint") {
      predicate = slide_show_title_predicate;
      ignore_original_window = true;
    } else if (name == "Keynote") {
      predicate = equal_title_predicate;
    } else if (owner_name == "OpenOffice") {
      return std::make_unique<open_office_application_handler>(source_id);
    }

    if (predicate) {
      result.reset(
          new full_screen_mac_application_handler(source_id, predicate, ignore_original_window));
    }
  }

  return result;
}

} // namespace base
} // namespace traa
