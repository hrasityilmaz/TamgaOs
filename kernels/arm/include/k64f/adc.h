#ifndef ADC_H
#define ADC_H

#include <stdint.h>

void adc_init(void);
uint16_t adc_read_se12(void);
uint8_t adc_calibration_failed(void);

#endif /* ADC_H */