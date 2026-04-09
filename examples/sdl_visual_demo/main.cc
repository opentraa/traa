#include "app.h"

#include <cstdio>

int main(int /*argc*/, char * /*argv*/[]) {
  app application;
  if (!application.init()) {
    fprintf(stderr, "Failed to initialize application\n");
    return 1;
  }
  application.run();
  application.shutdown();
  return 0;
}
