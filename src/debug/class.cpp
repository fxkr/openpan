#include <stdbool.h>

#include <mbed.h>

#include <error_handler.h>

#include "class.h"

namespace app::debug {

Debug::Debug(Serial &console) : console(console) {}

void Debug::printf(const char *format, ...) {
  va_list argptr;
  va_start(argptr, format);
  console.vprintf(format, argptr);
  va_end(argptr);
}

void Debug::do_crash(const char *func, const char *file, int line) {
  console.printf("\nCrash in %s (%s:%d)\n", func, file, line);

  // Call debugger or trigger hard fault
  __asm__("BKPT");

  // We won't return from bkpt, but to be sure
  while (true) {
  }
}

}  // namespace app::debug
