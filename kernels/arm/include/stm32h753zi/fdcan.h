#ifndef FDCAN_H
#define FDCAN_H

#include <stdint.h>

/* FDCAN frame type */
typedef struct {
    uint32_t id;        /* 11-bit CAN ID */
    uint8_t  dlc;       /* data length 0-8 */
    uint8_t  data[8];
} fdcan_frame_t;

void    fdcan_init(void);
int8_t  fdcan_transmit(const fdcan_frame_t *frame);
int8_t  fdcan_receive(fdcan_frame_t *frame);
uint8_t fdcan_rx_pending(void);
uint8_t fdcan_is_bus_off(void);
uint32_t fdcan_get_psr(void);
void fdcan_debug_print_gpio(void);

#endif /* FDCAN_H */