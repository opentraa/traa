#include <traa/traa.h>

#include <stdio.h>

void on_error(traa_userdata userdata, traa_error error_code, const char *context) {
  printf("userdata: %p, error_code: %d, context: %s\n", userdata, error_code, context);
}

int main() {
  traa_config config;

  // set the userdata.
  config.userdata = reinterpret_cast<traa_userdata>(0x12345678);

  // set the log config.
  config.log_config.log_file = "./traa.log";

  // set the event handler.
  config.event_handler.on_error = on_error;

  // initialize the traa library.
  traa_init(&config);

  // release the traa library.
  traa_release();

  return 0;
}
