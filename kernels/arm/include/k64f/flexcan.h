#ifndef FLEXCAN_H
#define FLEXCAN_H

#include <stdint.h>

typedef struct {
    uint32_t id;        /* 11-bit standard CAN ID */
    uint8_t  dlc;        /* data length 0-8 */
    uint8_t  data[8];
} flexcan_frame_t;

void    flexcan_init(void);
int8_t  flexcan_transmit(const flexcan_frame_t *frame);
int8_t  flexcan_receive(flexcan_frame_t *frame);
uint8_t flexcan_rx_pending(void);

#endif /* FLEXCAN_H */