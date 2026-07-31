#ifndef BMP280_H
#define BMP280_H

#include <stdint.h>

#define BMP280_ADDR  0x76U 

typedef struct {
    int32_t temperature_x100;
    uint32_t pressure_pa_x256;
    float    altitude_m;
} bmp280_data_t;

int8_t bmp280_init(void);
int8_t bmp280_read(bmp280_data_t *data);
int8_t bmp280_zero_altitude(uint8_t num_samples);

#endif /* BMP280_H */