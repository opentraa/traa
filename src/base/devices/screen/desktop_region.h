#ifndef TRAA_BASE_DEVICES_SCREEN_DESKTOP_REGION_H_
#define TRAA_BASE_DEVICES_SCREEN_DESKTOP_REGION_H_

#include <traa/base.h>

#include "base/devices/screen/desktop_geometry.h"

#include <map>
#include <vector>

namespace traa {
namespace base {

// desktop_region represents a region of the screen or window.
//
// Internally each region is stored as a set of rows where each row contains one
// or more rectangles aligned vertically.
class desktop_region {
private:
  // The following private types need to be declared first because they are used
  // in the public iterator.

  // row_span represents a horizontal span withing a single row.
  struct row_span {
    row_span(int32_t left, int32_t right);

    // Used by std::vector<>.
    bool operator==(const row_span &that) const { return left == that.left && right == that.right; }

    int32_t left;
    int32_t right;
  };

  using row_span_set = std::vector<row_span>;

  // row represents a single row of a region. A row is set of rectangles that
  // have the same vertical position.
  struct row {
    row(const row &);
    row(row &&);
    row(int32_t top, int32_t bottom);
    ~row();

    int32_t top;
    int32_t bottom;

    row_span_set spans;
  };

  // Type used to store list of rows in the region. The bottom position of row
  // is used as the key so that rows are always ordered by their position. The
  // map stores pointers to make Translate() more efficient.
  typedef std::map<int, row *> rows;

public:
  // iterator that can be used to iterate over rectangles of a desktop_region.
  // The region must not be mutated while the iterator is used.
  class iterator {
  public:
    explicit iterator(const desktop_region &target);
    ~iterator();

    bool is_at_end() const;
    void advance();

    const desktop_rect &rect() const { return rect_; }

  private:
    const desktop_region &region_;

    // Updates `rect_` based on the current `row_` and `row_span_`. If
    // `row_span_` matches spans on consecutive rows then they are also merged
    // into `rect_`, to generate more efficient output.
    void update_current_rect();

    rows::const_iterator row_;
    rows::const_iterator previous_row_;
    row_span_set::const_iterator row_span_;
    desktop_rect rect_;
  };

  desktop_region();
  explicit desktop_region(const desktop_rect &rect);
  desktop_region(const desktop_rect *rects, int count);
  desktop_region(const desktop_region &other);
  ~desktop_region();

  desktop_region &operator=(const desktop_region &other);

  bool is_empty() const { return rows_.empty(); }

  bool equals(const desktop_region &region) const;

  // Reset the region to be empty.
  void clear();

  // Reset region to contain just `rect`.
  void set_rect(const desktop_rect &rect);

  // Adds specified rect(s) or region to the region.
  void add_rect(const desktop_rect &rect);
  void add_rects(const desktop_rect *rects, int count);
  void add_region(const desktop_region &region);

  // Finds intersection of two regions and stores them in the current region.
  void intersect(const desktop_region &region1, const desktop_region &region2);

  // Same as above but intersects content of the current region with `region`.
  void intersect_with(const desktop_region &region);

  // Clips the region by the `rect`.
  void intersect_with(const desktop_rect &rect);

  // Subtracts `region` from the current content of the region.
  void subtract(const desktop_region &region);

  // Subtracts `rect` from the current content of the region.
  void subtract(const desktop_rect &rect);

  // Adds (dx, dy) to the position of the region.
  void translate(int32_t dx, int32_t dy);

  void swap(desktop_region *region);

private:
  // Comparison functions used for std::lower_bound(). Compare left or right
  // edges withs a given `value`.
  static bool compare_span_left(const row_span &r, int32_t value);
  static bool compare_span_right(const row_span &r, int32_t value);

  // Adds a new span to the row, coalescing spans if necessary.
  static void add_span_to_row(row *r, int32_t left, int32_t right);

  // Returns true if the `span` exists in the given `row`.
  static bool is_span_in_row(const row &r, const row_span &rect);

  // Calculates the intersection of two sets of spans.
  static void intersect_rows(const row_span_set &set1, const row_span_set &set2,
                             row_span_set *output);

  static void subtract_rows(const row_span_set &set_a, const row_span_set &set_b,
                            row_span_set *output);

  // Merges `row` with the row above it if they contain the same spans. Doesn't
  // do anything if called with `row` set to rows_.begin() (i.e. first row of
  // the region). If the rows were merged `row` remains a valid iterator to the
  // merged row.
  void merge_with_preceding_row(rows::iterator r);

  rows rows_;
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_DESKTOP_REGION_H_