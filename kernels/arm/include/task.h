#ifndef TASK_H
#define TASK_H

#include <stdint.h>

//
// 2KB x 8
//
#define TASK_MAX (8U)
#define TASK_STACK_SIZE (640U)

#define TASK_PRIORITY_HIGH (0U)
#define TASK_PRIORITY_NORMAL (1U)
#define TASK_PRIORITY_LOW (2U)
#define TASK_PRIORITY_IDLE (3U)

typedef enum {
  TASK_READY = 0U,
  TASK_RUNNING = 1U,
  TASK_BLOCKED = 2U,
  TASK_PAUSED = 3U,
  TASK_DEAD = 4U
} task_state_t;

typedef struct task_s {
  uint32_t *sp;
  task_state_t state;
  uint8_t priority;
  uint32_t delay_ticks;
  void (*func)(void);
  struct task_s *wait_next;
  uint32_t stack[TASK_STACK_SIZE] __attribute__((aligned(32)));
} task_t;

#endif
