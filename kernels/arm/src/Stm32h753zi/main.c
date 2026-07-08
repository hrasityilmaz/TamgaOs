#include "rcc.h"
#include "systick.h"
#include "uart.h"
#include "scheduler.h"
#include "task.h"
//#include <stdint.h>
#include "i2c.h"
#include "mpu6050.h"



#define RCC_AHB4ENR (*(volatile uint32_t *)0x580244E0U)
#define GPIOB_MODER (*(volatile uint32_t *)0x58020400U)
#define GPIOB_ODR   (*(volatile uint32_t *)0x58020414U)
#define GPIOE_MODER (*(volatile uint32_t *)0x58021000U)
#define GPIOE_ODR   (*(volatile uint32_t *)0x58021014U)

#define LD1_PIN (1UL << 0U)
#define LD2_PIN (1UL << 1U)
#define LD3_PIN (1UL << 14U)


// I2C
#define MPU6050_ADDR    0x69U
#define MPU6050_WHO_AM_I 0x75U


static void board_init(void)
{
    RCC_AHB4ENR |= (1UL << 1U) | (1UL << 4U);
    GPIOB_MODER &= ~((3UL << (0U  * 2U)) | (3UL << (14U * 2U)));
    GPIOB_MODER |=   (1UL << (0U  * 2U)) | (1UL << (14U * 2U));
    GPIOE_MODER &= ~(3UL << (1U * 2U));
    GPIOE_MODER |=   1UL << (1U * 2U);
    GPIOB_ODR &= ~(LD1_PIN | LD3_PIN);
    GPIOE_ODR &= ~LD2_PIN;
}


void task_imu(void)
{
    mpu6050_data_t data;
    while (1) {
        sched_delay_ms(10U);
        if (mpu6050_read(&data) == 0) {
            uart_printf("AX:%d  AY:%d  AZ:%d  |  GX:%d  GY:%d  GZ:%d\r\n",
                        data.accel_x, data.accel_y, data.accel_z,
                        data.gyro_x,  data.gyro_y,  data.gyro_z);
        }
    }
}


/* Task A — float accumulates up, should always increase */
/*
void task_float_a(void)
{
    float val = 0.0f;
    while (1) {
        val += 0.1f;
        GPIOB_ODR ^= LD1_PIN;
        sched_delay_ms(500U);
        int i = (int)val;
        int f = (int)((val - (float)i) * 1000.0f);
        if (f < 0) f = -f;
        uart_printf("[A] %d.%d\r\n", i, f);
    }
}
*/
/* Task B — float decreases down, should always decrease */
/*
void task_float_b(void)
{
    float val = 100.0f;
    while (1) {
        val -= 0.3f;
        GPIOB_ODR ^= LD3_PIN;
        sched_delay_ms(700U);
        int i = (int)val;
        int f = (int)((val - (float)i) * 1000.0f);
        if (f < 0) f = -f;
        uart_printf("[B] %d.%d\r\n", i, f);
    }
}
*/

/* Task LED — no float */
/*
void task_led_yellow(void)
{
    while (1) {
        GPIOE_ODR ^= LD2_PIN;
        sched_delay_ms(250U);
    }
}
*/

void task_led_green(void)
{
    while (1) {
        GPIOB_ODR ^= LD1_PIN;
        sched_delay_ms(500U);
    }
}
 
void task_led_yellow(void)
{
    while (1) {
        GPIOE_ODR ^= LD2_PIN;
        sched_delay_ms(300U);
    }
}
 
void task_led_red(void)
{
    while (1) {
        GPIOB_ODR ^= LD3_PIN;
        sched_delay_ms(700U);
    }
}

int main(void)
{
    rcc_init_pll_480();
    systick_init(480000000U);
    board_init();
    uart_init();
    i2c_init();

    uart_puts("TamgaOS STM32H753ZI @ 480MHz\r\n");
    //uart_puts("FPU context isolation test\r\n");
    //uart_puts("I2C MPU6050 Test");
    uart_puts("MPU6050 raw data and 3 Led task together\r\n");

    if (mpu6050_init() < 0) {
        uart_puts("MPU6050 init failed!\r\n");
        while (1) {}
    }

    uart_puts("MPU6050 OK\r\n");

    /*
    while (1) {
        if (mpu6050_read(&data) == 0) {
            uart_printf("AX:%d  AY:%d  AZ:%d  |  GX:%d  GY:%d  GZ:%d\r\n",
                        data.accel_x, data.accel_y, data.accel_z,
                        data.gyro_x,  data.gyro_y,  data.gyro_z);
        }
        systick_delay_ms(100U);
    }
    */


    /*
    uint8_t who = 0U;
    int8_t ret = i2c_read(MPU6050_ADDR, MPU6050_WHO_AM_I, &who, 1U);
    
    if (ret < 0) {
        uart_puts("I2C error — check wiring\r\n");
    } else {
        uart_printf("WHO_AM_I = 0x%x\r\n", who);
        if (who == 0x68U) {
            uart_puts("MPU6050 found!\r\n");
        } else {
            uart_puts("Unexpected device ID\r\n");
        }
    }
    */


    sched_init();

    //sched_task_create(task_float_a,    TASK_PRIORITY_NORMAL);
    //sched_task_create(task_float_b,    TASK_PRIORITY_NORMAL);
    //sched_task_create(task_led_yellow, TASK_PRIORITY_NORMAL);

    sched_task_create(task_imu,        TASK_PRIORITY_HIGH); // flight data :)
    sched_task_create(task_led_green,  TASK_PRIORITY_NORMAL);
    sched_task_create(task_led_yellow, TASK_PRIORITY_NORMAL);
    sched_task_create(task_led_red,    TASK_PRIORITY_NORMAL);

    sched_start();

    return 0;
}