#ifndef TASK_H
#define TASK_H

#include <stdint.h>

#define TASK_MAX        (4U)   /* 8'den 4'e indirdik */
#define TASK_STACK_SIZE (128U) /* 256'dan 128'e indirdik - 512 byte */

#define TASK_PRIORITY_HIGH   (0U)
#define TASK_PRIORITY_NORMAL (1U)
#define TASK_PRIORITY_LOW    (2U)
#define TASK_PRIORITY_IDLE   (3U)

typedef enum {
    TASK_READY   = 0U,
    TASK_RUNNING = 1U,
    TASK_BLOCKED = 2U,
    TASK_PAUSED  = 3U,
    TASK_DEAD    = 4U
} task_state_t;

typedef struct {
    uint32_t    *sp;
    task_state_t state;
    uint8_t      priority;
    uint32_t     delay_ticks;
    void        (*func)(void);
    uint32_t     stack[TASK_STACK_SIZE];
} task_t;

#endif