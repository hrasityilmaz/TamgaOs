/*
 * Priority Inheritance Test
 *
 * LOW task: retrieves mutex, holds for 1000ms, releases
 * HIGH task: attempts to retrieve mutex → LOW's priority is increased to HIGH
 * MED task: runs normally, should not overtake HIGH while waiting for it
 *
 * Beklenen çıktı:
 *   [LOW]  mutex <-
 *   [LOW]  priority up
 *   [LOW]  mutex ->
 *   [HIGH] mutex <-
 *   [MED]  working (after LOW)
 */

#include "rcc.h"
#include "systick.h"
#include "uart.h"
#include "scheduler.h"
#include "task.h"
#include "mutex.h"

#define RCC_AHB4ENR (*(volatile uint32_t *)0x580244E0U)
#define GPIOB_MODER (*(volatile uint32_t *)0x58020400U)
#define GPIOB_ODR   (*(volatile uint32_t *)0x58020414U)
#define GPIOE_MODER (*(volatile uint32_t *)0x58021000U)
#define GPIOE_ODR   (*(volatile uint32_t *)0x58021014U)

#define LD1_PIN (1UL << 0U)
#define LD2_PIN (1UL << 1U)
#define LD3_PIN (1UL << 14U)

static mutex_t g_test_mutex;

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

void task_low(void)
{
    sched_delay_ms(100U);
    while (1) {
        uart_puts("[LOW]  mutex getting...\r\n");
        mutex_lock(&g_test_mutex);
        GPIOB_ODR |= LD3_PIN;
        uart_puts("[LOW]  mutex get — busy for ~1000ms\r\n");
        for (volatile uint32_t i = 0; i < 30000000U; i++) {
            __asm volatile("nop");
        }

        uart_puts("[LOW]  mutex giving back\r\n");
        GPIOB_ODR &= ~LD3_PIN;
        mutex_unlock(&g_test_mutex);
        sched_delay_ms(500U);
    }
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

void task_med(void)
{
    int cnt = 0;
    while (1) {
        cnt++;
        GPIOE_ODR ^= LD2_PIN;
        uart_printf("[MED]  tick %d\r\n", cnt);
        sched_delay_ms(200U);
    }
}

void task_high(void)
{
    sched_delay_ms(300U); 
    while (1) {
        uart_puts("[HIGH] mutex getting\r\n");
        mutex_lock(&g_test_mutex);
        GPIOB_ODR |= LD1_PIN;
        uart_puts("[HIGH] mutex get \r\n");
        sched_delay_ms(200U);
        uart_puts("[HIGH] mutex giving back\r\n");
        GPIOB_ODR &= ~LD1_PIN;
        mutex_unlock(&g_test_mutex);
        sched_delay_ms(700U); 
    }
}

int main(void)
{
    rcc_init_pll_480();
    systick_init(480000000U);
    board_init();
    uart_init();

    uart_puts("TamgaOS STM32H753ZI @ 480MHz\r\n");
     uart_puts("Priority Inheritance Test\r\n");
    uart_puts("LOW=red  MED=yellow  HIGH=green\r\n");
    
    sched_init();
    sched_task_create(task_low,  TASK_PRIORITY_LOW);
    sched_task_create(task_med,  TASK_PRIORITY_NORMAL);
    sched_task_create(task_high, TASK_PRIORITY_HIGH);
    sched_start();

    return 0;
}
