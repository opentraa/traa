#include "panel_manager.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <random>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// mock_panel — tracks lifecycle and dispatch calls for property testing
// ---------------------------------------------------------------------------

class mock_panel : public panel_base {
public:
  explicit mock_panel(const std::string &name,
                      std::vector<std::string> *call_log = nullptr)
      : name_(name), call_log_(call_log) {}

  const char *get_name() const override { return name_.c_str(); }

  void on_enter() override {
    ++enter_count_;
    if (call_log_) {
      call_log_->push_back(name_ + "::on_enter");
    }
  }

  void on_leave() override {
    ++leave_count_;
    if (call_log_) {
      call_log_->push_back(name_ + "::on_leave");
    }
  }

  void on_render(SDL_Renderer * /*renderer*/, const SDL_FRect & /*area*/) override {
    ++render_count_;
    if (call_log_) {
      call_log_->push_back(name_ + "::on_render");
    }
  }

  bool on_event(const SDL_Event & /*event*/) override {
    ++event_count_;
    if (call_log_) {
      call_log_->push_back(name_ + "::on_event");
    }
    return false;
  }

  void on_update() override {
    ++update_count_;
    if (call_log_) {
      call_log_->push_back(name_ + "::on_update");
    }
  }

  // counters
  int enter_count_ = 0;
  int leave_count_ = 0;
  int render_count_ = 0;
  int event_count_ = 0;
  int update_count_ = 0;

private:
  std::string name_;
  std::vector<std::string> *call_log_;
};

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------

static std::string random_name(std::mt19937 &rng, int len) {
  static const char charset[] = "abcdefghijklmnopqrstuvwxyz0123456789";
  std::uniform_int_distribution<int> dist(0, sizeof(charset) - 2);
  std::string s;
  s.reserve(len);
  for (int i = 0; i < len; ++i) {
    s.push_back(charset[dist(rng)]);
  }
  return s;
}

// ===========================================================================
// Feature: sdl-visual-demo, Property 2: Panel 注册不变量
// Validates: Requirements 4.2, 14.3
//
// For any N (1-20) panels with unique names, after registration:
//   panel_count() == N and panel_name(i) matches registration order.
// ===========================================================================

TEST(PanelManagerProperty, RegistrationInvariant) {
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> count_dist(1, 20);
  std::uniform_int_distribution<int> len_dist(3, 12);

  for (int iter = 0; iter < 100; ++iter) {
    panel_manager mgr;
    int n = count_dist(rng);

    // generate unique names
    std::vector<std::string> names;
    names.reserve(n);
    for (int i = 0; i < n; ++i) {
      std::string name;
      do {
        name = random_name(rng, len_dist(rng));
      } while (std::find(names.begin(), names.end(), name) != names.end());
      names.push_back(name);
    }

    // register panels
    for (int i = 0; i < n; ++i) {
      mgr.register_panel(std::make_unique<mock_panel>(names[i]));
    }

    // verify count
    ASSERT_EQ(mgr.panel_count(), n)
        << "iter=" << iter << " expected " << n << " panels";

    // verify name ordering
    for (int i = 0; i < n; ++i) {
      ASSERT_STREQ(mgr.panel_name(i), names[i].c_str())
          << "iter=" << iter << " index=" << i;
    }
  }
}

// ===========================================================================
// Feature: sdl-visual-demo, Property 3: Panel 切换生命周期顺序
// Validates: Requirements 4.3
//
// For any registered panels and switch sequence, on_leave() of the old panel
// is called before on_enter() of the new panel, and active_index() updates.
// ===========================================================================

TEST(PanelManagerProperty, SwitchLifecycleOrder) {
  std::mt19937 rng(123);
  std::uniform_int_distribution<int> count_dist(2, 10);

  for (int iter = 0; iter < 100; ++iter) {
    int n = count_dist(rng);

    std::vector<std::string> call_log;
    panel_manager mgr;

    std::vector<std::string> names;
    for (int i = 0; i < n; ++i) {
      names.push_back("p" + std::to_string(i));
      mgr.register_panel(std::make_unique<mock_panel>(names.back(), &call_log));
    }

    // The first register_panel triggers switch_to(0), so panel 0 is active.
    // Clear the log so we only observe explicit switches below.
    call_log.clear();

    // perform random switches
    std::uniform_int_distribution<int> idx_dist(0, n - 1);
    int current = 0; // panel 0 is active after registration

    int num_switches = std::uniform_int_distribution<int>(1, 8)(rng);
    for (int s = 0; s < num_switches; ++s) {
      int target = idx_dist(rng);
      if (target == current) {
        // switch_to same index is a no-op per implementation
        size_t log_before = call_log.size();
        mgr.switch_to(target);
        ASSERT_EQ(call_log.size(), log_before)
            << "iter=" << iter << " switch to same index should be no-op";
        continue;
      }

      call_log.clear();
      mgr.switch_to(target);

      // Expect exactly two entries: old::on_leave then new::on_enter
      ASSERT_EQ(call_log.size(), 2u)
          << "iter=" << iter << " switch " << current << "->" << target;
      EXPECT_EQ(call_log[0], names[current] + "::on_leave")
          << "iter=" << iter;
      EXPECT_EQ(call_log[1], names[target] + "::on_enter")
          << "iter=" << iter;

      // active_index must be updated
      EXPECT_EQ(mgr.active_index(), target)
          << "iter=" << iter;

      current = target;
    }
  }
}

// ===========================================================================
// Feature: sdl-visual-demo, Property 4: 事件与渲染分发到活跃 Panel
// Validates: Requirements 4.4
//
// handle_event() and render() only invoke the active panel's methods;
// inactive panels receive zero calls.
// ===========================================================================

TEST(PanelManagerProperty, EventAndRenderDispatchToActivePanel) {
  std::mt19937 rng(999);
  std::uniform_int_distribution<int> count_dist(2, 10);

  for (int iter = 0; iter < 100; ++iter) {
    int n = count_dist(rng);

    panel_manager mgr;
    std::vector<mock_panel *> raw_ptrs;

    for (int i = 0; i < n; ++i) {
      auto p = std::make_unique<mock_panel>("p" + std::to_string(i));
      raw_ptrs.push_back(p.get());
      mgr.register_panel(std::move(p));
    }

    // pick a random active panel
    int active = std::uniform_int_distribution<int>(0, n - 1)(rng);
    if (active != 0) {
      mgr.switch_to(active);
    }

    // reset all counters (registration + switch may have triggered on_enter)
    for (auto *p : raw_ptrs) {
      p->event_count_ = 0;
      p->render_count_ = 0;
    }

    // dispatch a few events and renders
    SDL_Event dummy_event{};
    SDL_FRect dummy_area{0.0f, 0.0f, 800.0f, 600.0f};
    int dispatch_count = std::uniform_int_distribution<int>(1, 5)(rng);

    for (int d = 0; d < dispatch_count; ++d) {
      mgr.handle_event(dummy_event);
      mgr.render(nullptr, dummy_area);
    }

    // verify only the active panel received calls
    for (int i = 0; i < n; ++i) {
      if (i == active) {
        EXPECT_EQ(raw_ptrs[i]->event_count_, dispatch_count)
            << "iter=" << iter << " active panel " << i
            << " should have received " << dispatch_count << " events";
        EXPECT_EQ(raw_ptrs[i]->render_count_, dispatch_count)
            << "iter=" << iter << " active panel " << i
            << " should have received " << dispatch_count << " renders";
      } else {
        EXPECT_EQ(raw_ptrs[i]->event_count_, 0)
            << "iter=" << iter << " inactive panel " << i
            << " should have received 0 events";
        EXPECT_EQ(raw_ptrs[i]->render_count_, 0)
            << "iter=" << iter << " inactive panel " << i
            << " should have received 0 renders";
      }
    }
  }
}

// ===========================================================================
// Unit tests — edge cases and boundary conditions
// Validates: Requirements 4.2, 4.3, 4.4
// ===========================================================================

TEST(PanelManagerUnit, SwitchToInvalidIndex) {
  panel_manager mgr;
  std::vector<std::string> call_log;

  for (int i = 0; i < 3; ++i) {
    mgr.register_panel(
        std::make_unique<mock_panel>("p" + std::to_string(i), &call_log));
  }

  // after registration, panel 0 is active
  ASSERT_EQ(mgr.active_index(), 0);
  call_log.clear();

  // switch to negative index — should be ignored
  mgr.switch_to(-1);
  EXPECT_EQ(mgr.active_index(), 0);

  // switch to out-of-range index — should be ignored
  mgr.switch_to(100);
  EXPECT_EQ(mgr.active_index(), 0);

  // no lifecycle calls should have been made
  EXPECT_TRUE(call_log.empty());
}

TEST(PanelManagerUnit, SwitchToCurrentIndex) {
  panel_manager mgr;
  std::vector<std::string> call_log;

  mgr.register_panel(std::make_unique<mock_panel>("p0", &call_log));
  mgr.register_panel(std::make_unique<mock_panel>("p1", &call_log));

  // panel 0 is active after first registration
  ASSERT_EQ(mgr.active_index(), 0);
  call_log.clear();

  // switch to the already-active index — should be a no-op
  mgr.switch_to(0);
  EXPECT_EQ(mgr.active_index(), 0);
  EXPECT_TRUE(call_log.empty());
}

TEST(PanelManagerUnit, HandleEventWithNoPanels) {
  panel_manager mgr;
  SDL_Event dummy_event{};

  // should not crash when no panels are registered
  mgr.handle_event(dummy_event);
  EXPECT_EQ(mgr.active_index(), -1);
}

TEST(PanelManagerUnit, RenderWithNoPanels) {
  panel_manager mgr;
  SDL_FRect dummy_area{0.0f, 0.0f, 800.0f, 600.0f};

  // should not crash when no panels are registered
  mgr.render(nullptr, dummy_area);
  EXPECT_EQ(mgr.active_index(), -1);
}

TEST(PanelManagerUnit, UpdateWithNoPanels) {
  panel_manager mgr;

  // should not crash when no panels are registered
  mgr.update();
  EXPECT_EQ(mgr.active_index(), -1);
}
