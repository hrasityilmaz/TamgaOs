#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <stdint.h>

//
// TODO: Add wait list !!!!
//
typedef struct {
  volatile int32_t count;
  int32_t max;
} sem_t;

void sem_init(sem_t *s, int32_t initial, int32_t max);
void sem_take(sem_t *s);
void sem_give(sem_t *s);
int sem_trytake(sem_t *s);

#endif
