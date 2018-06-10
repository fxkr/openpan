#pragma once

#include <mbed.h>

namespace app::debug {

class Debug {
 private:
  Serial &console;

 public:
  Debug(Serial &console);
  void printf(const char *format, ...);
  void do_crash(const char *func, const char *file, int line);
};

}  // namespace app::debug
