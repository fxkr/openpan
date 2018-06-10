#ifndef __ETL_PROFILE_H__
#define __ETL_PROFILE_H__

// On errors, call error handler only. Ours will abort.
#define ETL_LOG_ERRORS

// Pushes and pops to containers are checked for bounds.
#define ETL_CHECK_PUSH_POP

// GCC generic settings. Generic target. No OS.
#include <profiles/gcc_generic.h>

#endif
