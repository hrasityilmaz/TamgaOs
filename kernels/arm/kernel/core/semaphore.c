#include "sched_critical.h"
#include "scheduler.h"
#include "semaphore.h"
#include <stdint.h>

void sem_init(sem_t *s, int32_t initial, int32_t max) {
  s->count = initial;
  s->max = max;
}

void sem_take(sem_t *s) {
  for (;;) {
    uint32_t p = sched_critical_enter();
    if (s->count > 0) {
      s->count--;
      sched_critical_exit(p);
      return;
    }

    sched_critical_exit(p);
    sched_delay_ms(1U);
  }
}

void sem_give(sem_t *s) {
  uint32_t p = sched_critical_enter();
  if (s->count < s->max) {
    s->count++;
  }
  sched_critical_exit(p);
}

int sem_trytake(sem_t *s) {
  uint32_t p = sched_critical_enter();

  if (s->count > 0) {
    s->count--;
    sched_critical_exit(p);
    return 1;
  }

  sched_critical_exit(p);
  return 0;
}
