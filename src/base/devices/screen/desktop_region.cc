/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/desktop_region.h"

#include <algorithm>
#include <utility>

namespace traa {
namespace base {

desktop_region::row_span::row_span(int32_t left, int32_t right) : left(left), right(right) {}

desktop_region::row::row(const row &) = default;
desktop_region::row::row(row &&) = default;

desktop_region::row::row(int32_t top, int32_t bottom) : top(top), bottom(bottom) {}

desktop_region::row::~row() {}

desktop_region::desktop_region() {}

desktop_region::desktop_region(const desktop_rect &rect) { add_rect(rect); }

desktop_region::desktop_region(const desktop_rect *rects, int count) { add_rects(rects, count); }

desktop_region::desktop_region(const desktop_region &other) { *this = other; }

desktop_region::~desktop_region() { clear(); }

desktop_region &desktop_region::operator=(const desktop_region &other) {
  clear();
  rows_ = other.rows_;
  for (rows_t::iterator it = rows_.begin(); it != rows_.end(); ++it) {
    // Copy each row.
    row *r = it->second;
    it->second = new row(*r);
  }
  return *this;
}

bool desktop_region::equals(const desktop_region &region) const {
  // Iterate over rows of the tow regions and compare each row.
  rows_t::const_iterator it1 = rows_.begin();
  rows_t::const_iterator it2 = region.rows_.begin();
  while (it1 != rows_.end()) {
    if (it2 == region.rows_.end() || it1->first != it2->first ||
        it1->second->top != it2->second->top || it1->second->bottom != it2->second->bottom ||
        it1->second->spans != it2->second->spans) {
      return false;
    }
    ++it1;
    ++it2;
  }
  return it2 == region.rows_.end();
}

void desktop_region::clear() {
  for (rows_t::iterator r = rows_.begin(); r != rows_.end(); ++r) {
    delete r->second;
  }
  rows_.clear();
}

void desktop_region::set_rect(const desktop_rect &rect) {
  clear();
  add_rect(rect);
}

void desktop_region::add_rect(const desktop_rect &rect) {
  if (rect.is_empty())
    return;

  // Top of the part of the `rect` that hasn't been inserted yet. Increased as
  // we iterate over the rows until it reaches `rect.bottom()`.
  int top = rect.top();

  // Iterate over all rows that may intersect with `rect` and add new rows when
  // necessary.
  rows_t::iterator r = rows_.upper_bound(top);
  while (top < rect.bottom()) {
    if (r == rows_.end() || top < r->second->top) {
      // If `top` is above the top of the current `row` then add a new row above
      // the current one.
      int32_t bottom = rect.bottom();
      if (r != rows_.end() && r->second->top < bottom)
        bottom = r->second->top;
      r = rows_.insert(r, rows_t::value_type(bottom, new row(top, bottom)));
    } else if (top > r->second->top) {
      // If the `top` falls in the middle of the `row` then split `row` into
      // two, at `top`, and leave `row` referring to the lower of the two,
      // ready to insert a new span into.
      rows_t::iterator new_r =
          rows_.insert(r, rows_t::value_type(top, new row(r->second->top, top)));
      r->second->top = top;
      new_r->second->spans = r->second->spans;
    }

    if (rect.bottom() < r->second->bottom) {
      // If the bottom of the `rect` falls in the middle of the `row` split
      // `row` into two, at `top`, and leave `row` referring to the upper of
      // the two, ready to insert a new span into.
      rows_t::iterator new_r =
          rows_.insert(r, rows_t::value_type(rect.bottom(), new row(top, rect.bottom())));
      r->second->top = rect.bottom();
      new_r->second->spans = r->second->spans;
      r = new_r;
    }

    // Add a new span to the current row.
    add_span_to_row(r->second, rect.left(), rect.right());
    top = r->second->bottom;

    merge_with_preceding_row(r);

    // Move to the next row.
    ++r;
  }

  if (r != rows_.end())
    merge_with_preceding_row(r);
}

void desktop_region::add_rects(const desktop_rect *rects, int count) {
  for (int i = 0; i < count; ++i) {
    add_rect(rects[i]);
  }
}

void desktop_region::merge_with_preceding_row(rows_t::iterator r) {
  if (r != rows_.begin()) {
    rows_t::iterator previous_r = r;
    previous_r--;

    // If `row` and `previous_r` are next to each other and contain the same
    // set of spans then they can be merged.
    if (previous_r->second->bottom == r->second->top &&
        previous_r->second->spans == r->second->spans) {
      r->second->top = previous_r->second->top;
      delete previous_r->second;
      rows_.erase(previous_r);
    }
  }
}

void desktop_region::add_region(const desktop_region &region) {
  // TODO(sergeyu): This function is not optimized - potentially it can iterate
  // over rows of the two regions similar to how it works in intersect().
  for (iterator it(region); !it.is_at_end(); it.advance()) {
    add_rect(it.rect());
  }
}

void desktop_region::intersect(const desktop_region &region1, const desktop_region &region2) {
  clear();

  rows_t::const_iterator it1 = region1.rows_.begin();
  rows_t::const_iterator end1 = region1.rows_.end();
  rows_t::const_iterator it2 = region2.rows_.begin();
  rows_t::const_iterator end2 = region2.rows_.end();
  if (it1 == end1 || it2 == end2)
    return;

  while (it1 != end1 && it2 != end2) {
    // Arrange for `it1` to always be the top-most of the rows.
    if (it2->second->top < it1->second->top) {
      std::swap(it1, it2);
      std::swap(end1, end2);
    }

    // Skip `it1` if it doesn't intersect `it2` at all.
    if (it1->second->bottom <= it2->second->top) {
      ++it1;
      continue;
    }

    // Top of the `it1` row is above the top of `it2`, so top of the
    // intersection is always the top of `it2`.
    int32_t top = it2->second->top;
    int32_t bottom = std::min(it1->second->bottom, it2->second->bottom);

    rows_t::iterator new_row =
        rows_.insert(rows_.end(), rows_t::value_type(bottom, new row(top, bottom)));
    intersect_rows(it1->second->spans, it2->second->spans, &new_row->second->spans);
    if (new_row->second->spans.empty()) {
      delete new_row->second;
      rows_.erase(new_row);
    } else {
      merge_with_preceding_row(new_row);
    }

    // If `it1` was completely consumed, move to the next one.
    if (it1->second->bottom == bottom)
      ++it1;
    // If `it2` was completely consumed, move to the next one.
    if (it2->second->bottom == bottom)
      ++it2;
  }
}

// static
void desktop_region::intersect_rows(const row_span_set_t &set1, const row_span_set_t &set2,
                                    row_span_set_t *output) {
  row_span_set_t::const_iterator it1 = set1.begin();
  row_span_set_t::const_iterator end1 = set1.end();
  row_span_set_t::const_iterator it2 = set2.begin();
  row_span_set_t::const_iterator end2 = set2.end();

  do {
    // Arrange for `it1` to always be the left-most of the spans.
    if (it2->left < it1->left) {
      std::swap(it1, it2);
      std::swap(end1, end2);
    }

    // Skip `it1` if it doesn't intersect `it2` at all.
    if (it1->right <= it2->left) {
      ++it1;
      continue;
    }

    int32_t left = it2->left;
    int32_t right = std::min(it1->right, it2->right);

    output->push_back(row_span(left, right));

    // If `it1` was completely consumed, move to the next one.
    if (it1->right == right)
      ++it1;
    // If `it2` was completely consumed, move to the next one.
    if (it2->right == right)
      ++it2;
  } while (it1 != end1 && it2 != end2);
}

void desktop_region::intersect_with(const desktop_region &region) {
  desktop_region old_region;
  swap(&old_region);
  intersect(old_region, region);
}

void desktop_region::intersect_with(const desktop_rect &rect) {
  desktop_region region;
  region.add_rect(rect);
  intersect_with(region);
}

void desktop_region::subtract(const desktop_region &region) {
  if (region.rows_.empty())
    return;

  // `row_b` refers to the current row being subtracted.
  rows_t::const_iterator row_b = region.rows_.begin();

  // Current vertical position at which subtraction is happening.
  int top = row_b->second->top;

  // `row_a` refers to the current row we are subtracting from. Skip all rows
  // above `top`.
  rows_t::iterator row_a = rows_.upper_bound(top);

  // Step through rows of the both regions subtracting content of `row_b` from
  // `row_a`.
  while (row_a != rows_.end() && row_b != region.rows_.end()) {
    // Skip `row_a` if it doesn't intersect with the `row_b`.
    if (row_a->second->bottom <= top) {
      // Each output row is merged with previously-processed rows before further
      // rows are processed.
      merge_with_preceding_row(row_a);
      ++row_a;
      continue;
    }

    if (top > row_a->second->top) {
      // If `top` falls in the middle of `row_a` then split `row_a` into two, at
      // `top`, and leave `row_a` referring to the lower of the two, ready to
      // subtract spans from.
      rows_t::iterator new_row =
          rows_.insert(row_a, rows_t::value_type(top, new row(row_a->second->top, top)));
      row_a->second->top = top;
      new_row->second->spans = row_a->second->spans;
    } else if (top < row_a->second->top) {
      // If the `top` is above `row_a` then skip the range between `top` and
      // top of `row_a` because it's empty.
      top = row_a->second->top;
      if (top >= row_b->second->bottom) {
        ++row_b;
        if (row_b != region.rows_.end())
          top = row_b->second->top;
        continue;
      }
    }

    if (row_b->second->bottom < row_a->second->bottom) {
      // If the bottom of `row_b` falls in the middle of the `row_a` split
      // `row_a` into two, at `top`, and leave `row_a` referring to the upper of
      // the two, ready to subtract spans from.
      int bottom = row_b->second->bottom;
      rows_t::iterator new_row =
          rows_.insert(row_a, rows_t::value_type(bottom, new row(top, bottom)));
      row_a->second->top = bottom;
      new_row->second->spans = row_a->second->spans;
      row_a = new_row;
    }

    // At this point the vertical range covered by `row_a` lays within the
    // range covered by `row_b`. subtract `row_b` spans from `row_a`.
    row_span_set_t new_spans;
    subtract_rows(row_a->second->spans, row_b->second->spans, &new_spans);
    new_spans.swap(row_a->second->spans);
    top = row_a->second->bottom;

    if (top >= row_b->second->bottom) {
      ++row_b;
      if (row_b != region.rows_.end())
        top = row_b->second->top;
    }

    // Check if the row is empty after subtraction and delete it. Otherwise move
    // to the next one.
    if (row_a->second->spans.empty()) {
      rows_t::iterator row_to_delete = row_a;
      ++row_a;
      delete row_to_delete->second;
      rows_.erase(row_to_delete);
    } else {
      merge_with_preceding_row(row_a);
      ++row_a;
    }
  }

  if (row_a != rows_.end())
    merge_with_preceding_row(row_a);
}

void desktop_region::subtract(const desktop_rect &rect) {
  desktop_region region;
  region.add_rect(rect);
  subtract(region);
}

void desktop_region::translate(int32_t dx, int32_t dy) {
  rows_t new_rows;

  for (rows_t::iterator it = rows_.begin(); it != rows_.end(); ++it) {
    row *r = it->second;

    r->top += dy;
    r->bottom += dy;

    if (dx != 0) {
      // translate each span.
      for (row_span_set_t::iterator span = r->spans.begin(); span != r->spans.end(); ++span) {
        span->left += dx;
        span->right += dx;
      }
    }

    if (dy != 0)
      new_rows.insert(new_rows.end(), rows_t::value_type(r->bottom, r));
  }

  if (dy != 0)
    new_rows.swap(rows_);
}

void desktop_region::swap(desktop_region *region) { rows_.swap(region->rows_); }

// static
bool desktop_region::compare_span_right(const row_span &r, int32_t value) {
  return r.right < value;
}

// static
bool desktop_region::compare_span_left(const row_span &r, int32_t value) { return r.left < value; }

// static
void desktop_region::add_span_to_row(row *r, int left, int right) {
  // First check if the new span is located to the right of all existing spans.
  // This is an optimization to avoid binary search in the case when rectangles
  // are inserted sequentially from left to right.
  if (r->spans.empty() || left > r->spans.back().right) {
    r->spans.push_back(row_span(left, right));
    return;
  }

  // Find the first span that ends at or after `left`.
  row_span_set_t::iterator start =
      std::lower_bound(r->spans.begin(), r->spans.end(), left, compare_span_right);

  // Find the first span that starts after `right`.
  row_span_set_t::iterator end =
      std::lower_bound(start, r->spans.end(), right + 1, compare_span_left);
  if (end == r->spans.begin()) {
    // There are no overlaps. Just insert the new span at the beginning.
    r->spans.insert(r->spans.begin(), row_span(left, right));
    return;
  }

  // Move end to the left, so that it points the last span that ends at or
  // before `right`.
  end--;

  // At this point [start, end] is the range of spans that intersect with the
  // new one.
  if (end < start) {
    // There are no overlaps. Just insert the new span at the correct position.
    r->spans.insert(start, row_span(left, right));
    return;
  }

  left = std::min(left, start->left);
  right = std::max(right, end->right);

  // Replace range [start, end] with the new span.
  *start = row_span(left, right);
  ++start;
  ++end;
  if (start < end)
    r->spans.erase(start, end);
}

// static
bool desktop_region::is_span_in_row(const row &r, const row_span &span) {
  // Find the first span that starts at or after `span.left` and then check if
  // it's the same span.
  row_span_set_t::const_iterator it =
      std::lower_bound(r.spans.begin(), r.spans.end(), span.left, compare_span_left);
  return it != r.spans.end() && *it == span;
}

// static
void desktop_region::subtract_rows(const row_span_set_t &set_a, const row_span_set_t &set_b,
                                   row_span_set_t *output) {
  row_span_set_t::const_iterator it_b = set_b.begin();

  // Iterate over all spans in `set_a` adding parts of it that do not intersect
  // with `set_b` to the `output`.
  for (row_span_set_t::const_iterator it_a = set_a.begin(); it_a != set_a.end(); ++it_a) {
    // If there is no intersection then append the current span and continue.
    if (it_b == set_b.end() || it_a->right < it_b->left) {
      output->push_back(*it_a);
      continue;
    }

    // Iterate over `set_b` spans that may intersect with `it_a`.
    int pos = it_a->left;
    while (it_b != set_b.end() && it_b->left < it_a->right) {
      if (it_b->left > pos)
        output->push_back(row_span(pos, it_b->left));
      if (it_b->right > pos) {
        pos = it_b->right;
        if (pos >= it_a->right)
          break;
      }
      ++it_b;
    }
    if (pos < it_a->right)
      output->push_back(row_span(pos, it_a->right));
  }
}

desktop_region::iterator::iterator(const desktop_region &region)
    : region_(region), row_(region.rows_.begin()), previous_row_(region.rows_.end()) {
  if (!is_at_end()) {
    row_span_ = row_->second->spans.begin();
    update_current_rect();
  }
}

desktop_region::iterator::~iterator() {}

bool desktop_region::iterator::is_at_end() const { return row_ == region_.rows_.end(); }

void desktop_region::iterator::advance() {
  while (true) {
    ++row_span_;
    if (row_span_ == row_->second->spans.end()) {
      previous_row_ = row_;
      ++row_;
      if (row_ != region_.rows_.end()) {
        row_span_ = row_->second->spans.begin();
      }
    }

    if (is_at_end())
      return;

    // If the same span exists on the previous row then skip it, as we've
    // already returned this span merged into the previous one, via
    // update_current_rect().
    if (previous_row_ != region_.rows_.end() &&
        previous_row_->second->bottom == row_->second->top &&
        is_span_in_row(*previous_row_->second, *row_span_)) {
      continue;
    }

    break;
  }

  update_current_rect();
}

void desktop_region::iterator::update_current_rect() {
  // Merge the current rectangle with the matching spans from later rows.
  int bottom;
  rows_t::const_iterator bottom_row = row_;
  rows_t::const_iterator previous;
  do {
    bottom = bottom_row->second->bottom;
    previous = bottom_row;
    ++bottom_row;
  } while (bottom_row != region_.rows_.end() &&
           previous->second->bottom == bottom_row->second->top &&
           is_span_in_row(*bottom_row->second, *row_span_));
  rect_ = desktop_rect::make_ltrb(row_span_->left, row_->second->top, row_span_->right, bottom);
}

} // namespace base
} // namespace traa