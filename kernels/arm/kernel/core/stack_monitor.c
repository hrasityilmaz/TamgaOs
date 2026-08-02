/*
 * stack_monitor.c
 */

#include "stack_monitor.h"
#include <string.h>

#define STACK_SCAN_START_INDEX  (9U)

void stack_monitor_fill(task_t *t)
{
    if (t == NULL) {
        return;
    }

    uint32_t sp_index = (uint32_t)(t->sp - t->stack);

    if (sp_index <= STACK_SCAN_START_INDEX) {
        return;
    }

    memset(&t->stack[STACK_SCAN_START_INDEX], (int)STACK_FILL_BYTE,
           (size_t)(sp_index - STACK_SCAN_START_INDEX) * sizeof(uint32_t));
}

uint32_t stack_monitor_get_used_bytes(const task_t *t)
{
    if (t == NULL) {
        return 0U;
    }

    const uint8_t *bytes = (const uint8_t *)&t->stack[STACK_SCAN_START_INDEX];
    uint32_t region_words = TASK_STACK_SIZE - STACK_SCAN_START_INDEX;
    uint32_t region_bytes = region_words * sizeof(uint32_t);

    uint32_t untouched = 0U;
    while (untouched < region_bytes && bytes[untouched] == STACK_FILL_BYTE) {
        untouched++;
    }

    return region_bytes - untouched;
}

uint8_t stack_monitor_get_used_percent(const task_t *t)
{
    if (t == NULL) {
        return 0U;
    }

    uint32_t total_bytes = TASK_STACK_SIZE * (uint32_t)sizeof(uint32_t);
    uint32_t used_bytes  = stack_monitor_get_used_bytes(t);

    if (total_bytes == 0U) {
        return 0U;
    }

    return (uint8_t)((used_bytes * 100U) / total_bytes);
}