#include "rcc.h"
#include "systick.h"
#include "uart.h"
#include "scheduler.h"
#include "task.h"
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
            uart_print_imu(data.accel_x, data.accel_y, data.accel_z,
               data.gyro_x,  data.gyro_y,  data.gyro_z);
        }
    }
}



int main(void)
{
    rcc_init_pll_480();
    systick_init(480000000U);
    board_init();
    uart_init();
    i2c_init();
    mpu6050_init();

    uart_puts("TamgaOS STM32H753ZI @ 480MHz\r\n");
    
    sched_init();
    sched_task_create(task_imu, TASK_PRIORITY_HIGH); // flight data :)
    sched_start();

    return 0;
}
