#ifndef TRAA_BASE_DEVICES_SCREEN_LINUX_X11_X_ATOM_CACHE_H_
#define TRAA_BASE_DEVICES_SCREEN_LINUX_X11_X_ATOM_CACHE_H_

#include <X11/X.h>
#include <X11/Xlib.h>

namespace traa {
namespace base {

// A cache of Atom. Each Atom object is created on demand.
class x_atom_cache final {
public:
  explicit x_atom_cache(::Display *display);
  ~x_atom_cache();

  ::Display *display() const;

  Atom wm_state();
  Atom window_type();
  Atom window_type_normal();
  Atom icc_profile();

private:
  // If |*atom| is None, this function uses XInternAtom() to retrieve an Atom.
  Atom create_if_not_exist(Atom *atom, const char *name);

  ::Display *const display_;
  Atom wm_state_ = None;
  Atom window_type_ = None;
  Atom window_type_normal_ = None;
  Atom icc_profile_ = None;
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_LINUX_X11_X_ATOM_CACHE_H_