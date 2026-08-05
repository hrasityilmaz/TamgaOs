#ifndef ETH_H
#define ETH_H

#include <stdint.h>

void eth_init(uint8_t loopback_mode);
int8_t eth_transmit(const uint8_t *data, uint16_t len);
int8_t eth_receive(uint8_t *buf, uint16_t max_len, uint16_t *out_len);
uint16_t mdio_read(uint8_t phy_addr, uint8_t reg_addr);
void mdio_write(uint8_t phy_addr, uint8_t reg_addr, uint16_t value);
void eth_init_link_only(void);
// Just for test
uint32_t eth_debug_read_dmacsr(void); 
void eth_debug_clear_dmacsr(uint32_t mask);
uint32_t eth_debug_read_rx_des3(uint8_t index);
void eth_debug_dump_registers(void);


#endif /* ETH_H */