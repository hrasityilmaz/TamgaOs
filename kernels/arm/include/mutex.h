#ifndef MUTEX_H
#define MUTEX_H

#include "task.h"
#include <stddef.h>
#include <stdint.h>

typedef struct {
  // volatile task_t *task;
  task_t *volatile task;
  task_t *waiters;
} mutex_t;

void mutex_init(mutex_t *m);
void mutex_lock(mutex_t *m);
void mutex_unlock(mutex_t *m);
int mutex_trylock(mutex_t *m);

#endif
