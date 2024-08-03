#ifndef TRAA_MAIN_ENGINE_H
#define TRAA_MAIN_ENGINE_H

#include <traa/traa.h>

#include <string>
#include <vector>

#include "base/disallow.h"
#include "base/thread/callback.h"

namespace traa {
namespace main {

class engine : public base::support_weak_callback {
  DISALLOW_COPY_AND_ASSIGN(engine);

public:
  engine();
  ~engine();

  int init(const traa_config *config);

  int set_event_handler(const traa_event_handler *event_handler);

private:
};

} // namespace main
} // namespace traa

#endif // TRAA_MAIN_ENGINE_H