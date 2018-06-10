#include <stdbool.h>

#include <mbed.h>

#include <error_handler.h>

#include "class.h"

namespace app::debug {

// Singleton called by external libraries
static Debug *global_dbg;

void init(Debug &dbg) {
  // Set global debug singleton
  global_dbg = &dbg;

  // Handle Embedded Template Library errors
  etl::error_handler::free_function error_callback([](const etl::exception &e) {
    Debug *dbg = global_dbg;
    if (dbg) {
      dbg->do_crash(__func__, __FILE__, __LINE__);
    }
  });
  etl::error_handler::set_callback(error_callback);
}

void do_crash(const char *func, const char *file, int line) {
  Debug *dbg = global_dbg;
  if (dbg) {
    dbg->do_crash(func, file, line);
  }
}

}  // namespace app::debug
