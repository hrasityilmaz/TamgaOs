#ifndef MUTEX_H
#define MUTEX_H

#include "task.h"
#include <stddef.h>
#include <stdint.h>

typedef struct {
  // volatile task_t *task;
  task_t *volatile task;
  task_t *waiters;
  uint8_t owner_original_priority;  /* mutex alındığında asıl priority */
  uint8_t is_elevated; 
} mutex_t;

void mutex_init(mutex_t *m);
void mutex_lock(mutex_t *m);
void mutex_unlock(mutex_t *m);
int mutex_trylock(mutex_t *m);

#endif
