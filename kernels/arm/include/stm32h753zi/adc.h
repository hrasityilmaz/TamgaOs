#ifndef ADC_H
#define ADC_H

#include <stdint.h>

void adc_init(void);
uint16_t adc_read_channel15(void);

#endif /* ADC_H */