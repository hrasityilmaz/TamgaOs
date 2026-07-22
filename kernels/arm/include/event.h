#ifndef EVENT_H
#define EVENT_H

#include "task.h"
#include <stdint.h>

/*
 * Bitmask-based event group (up to 32 flags per group).
 *
 * A task can wait for:
 *   - ANY of a set of bits to become set (EVENT_WAIT_ANY)
 *   - ALL of a set of bits to become set (EVENT_WAIT_ALL)
 *
 */
typedef struct {
  volatile uint32_t bits;
  task_t *waiters;
} event_group_t;

typedef enum {
  EVENT_WAIT_ANY = 0,
  EVENT_WAIT_ALL = 1
} event_wait_mode_t;

void event_init(event_group_t *e);
void event_set(event_group_t *e, uint32_t bits_to_set);
void event_clear(event_group_t *e, uint32_t bits_to_clear);
uint32_t event_get(event_group_t *e);
uint32_t event_wait(event_group_t *e, uint32_t mask,
                     event_wait_mode_t mode, int auto_clear);
uint32_t event_wait_timeout(event_group_t *e, uint32_t mask,
                             event_wait_mode_t mode, int auto_clear,
                             uint32_t timeout_ms);

#endif /* EVENT_H */