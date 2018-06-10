#ifndef ECL_USER_INCLUDED
#define ECL_USER_INCLUDED

#include <stdint.h>

extern uint32_t timer_semaphore;

#define ECL_TIMER_DISABLE_PROCESSING __sync_fetch_and_add(&timer_semaphore, 1)
#define ECL_TIMER_ENABLE_PROCESSING __sync_fetch_and_sub(&timer_semaphore, 1)
#define ECL_TIMER_PROCESSING_ENABLED \
  (__sync_fetch_and_add(&timer_semaphore, 0) == 0)

#endif
