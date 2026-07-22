#ifndef QUEUE_H
#define QUEUE_H

#include "task.h"
#include <stddef.h>
#include <stdint.h>

typedef struct {
  uint8_t *buffer;
  uint16_t item_size;
  uint16_t capacity;
  uint16_t count;
  uint16_t head;
  uint16_t tail;
  task_t *waiters_send;
  task_t *waiters_receive;
} queue_t;

void queue_init(queue_t *q, void *buffer, uint16_t item_size, uint16_t capacity);
void queue_send(queue_t *q, const void *item);
void queue_receive(queue_t *q, void *item);
int queue_try_send(queue_t *q, const void *item);
int queue_try_receive(queue_t *q, void *item);

int queue_send_timeout(queue_t *q, const void *item, uint32_t timeout_ms);
int queue_receive_timeout(queue_t *q, void *item, uint32_t timeout_ms);
#define QUEUE_DEFINE(name, type, cap) \
  static type name##_storage[cap];    \
  static queue_t name

#endif /* QUEUE_H */