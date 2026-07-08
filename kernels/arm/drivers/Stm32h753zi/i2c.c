/*
 * i2c.c — STM32H753ZI I2C1 driver
 *
 * SCL → PB8 (AF4)
 * SDA → PB9 (AF4)
 *
 * APB1 = 240MHz
 * Speed = 400kHz (Fast Mode)
 *
 * TIMINGR = 0x00B03FDF (APB1=240MHz, FM 400kHz)
 *
 * REF: RM0433 Rev8
 *   I2C1 base:    0x40005400
 *   RCC_APB1LENR: 0x58024400 + 0x0E8, I2C1EN bit21
 *   GPIOB base:   0x58020400
 *   RCC_AHB4ENR:  0x58024400 + 0x0E0, GPIOBEN bit1
 */

 #include "i2c.h"

 #define I2C1_BASE    0x40005400UL
#define I2C1_CR1     (*(volatile uint32_t *)(I2C1_BASE + 0x00U))
#define I2C1_CR2     (*(volatile uint32_t *)(I2C1_BASE + 0x04U))
#define I2C1_TIMINGR (*(volatile uint32_t *)(I2C1_BASE + 0x10U))
#define I2C1_ISR     (*(volatile uint32_t *)(I2C1_BASE + 0x18U))
#define I2C1_ICR     (*(volatile uint32_t *)(I2C1_BASE + 0x1CU))
#define I2C1_RXDR    (*(volatile uint32_t *)(I2C1_BASE + 0x24U))
#define I2C1_TXDR    (*(volatile uint32_t *)(I2C1_BASE + 0x28U))

/* I2C_CR1 bits */
#define I2C_CR1_PE       (1UL << 0U)

/* I2C_CR2 bits */
#define I2C_CR2_SADD(x)   ((uint32_t)(x) << 1U)
#define I2C_CR2_RD_WRN    (1UL << 10U)
#define I2C_CR2_NBYTES(x) ((uint32_t)(x) << 16U)
#define I2C_CR2_AUTOEND   (1UL << 25U)
#define I2C_CR2_START     (1UL << 13U)
#define I2C_CR2_STOP      (1UL << 14U)

/* I2C_ISR bits */
#define I2C_ISR_TXIS    (1UL << 1U)
#define I2C_ISR_RXNE    (1UL << 2U)
#define I2C_ISR_TC      (1UL << 6U)
#define I2C_ISR_STOPF   (1UL << 5U)
#define I2C_ISR_NACKF   (1UL << 4U)
#define I2C_ISR_BUSY    (1UL << 15U)

/* I2C_ICR bits */
#define I2C_ICR_NACKCF  (1UL << 4U)
#define I2C_ICR_STOPCF  (1UL << 5U)

/* ── RCC ── */
#define RCC_BASE     0x58024400UL
#define RCC_AHB4ENR  (*(volatile uint32_t *)(RCC_BASE + 0x0E0U))
#define RCC_APB1LENR (*(volatile uint32_t *)(RCC_BASE + 0x0E8U))

/* ── GPIOB ── */
#define GPIOB_BASE   0x58020400UL
#define GPIOB_MODER   (*(volatile uint32_t *)(GPIOB_BASE + 0x00U))
#define GPIOB_OTYPER  (*(volatile uint32_t *)(GPIOB_BASE + 0x04U))
#define GPIOB_OSPEEDR (*(volatile uint32_t *)(GPIOB_BASE + 0x08U))
#define GPIOB_PUPDR   (*(volatile uint32_t *)(GPIOB_BASE + 0x0CU))
#define GPIOB_AFRH    (*(volatile uint32_t *)(GPIOB_BASE + 0x24U))

/* ── Timeout ── */
#define I2C_TIMEOUT  (100000U)

static int i2c_wait(volatile uint32_t *reg, uint32_t flag, uint32_t set){
    uint32_t t = I2C_TIMEOUT;
    while (t--) {
        if (set) {
            if (*reg & flag) return 0;
        } else {
            if (!(*reg & flag)) return 0;
        }
    }
    return -1;
}

void i2c_init(void){
    RCC_AHB4ENR |= (1UL << 1U);
    RCC_APB1LENR |= (1UL << 21U);
    /* 2. PB8=SCL, PB9=SDA → AF4, open-drain, very high speed, pull-up */
    /* MODER: AF mode = 10 */
    GPIOB_MODER &= ~((3UL << (8U * 2U)) | (3UL << (9U * 2U)));
    GPIOB_MODER |=   (2UL << (8U * 2U)) | (2UL << (9U * 2U));
    GPIOB_OTYPER |= (1UL << 8U) | (1UL << 9U);
    GPIOB_OSPEEDR |= (3UL << (8U * 2U)) | (3UL << (9U * 2U));
    GPIOB_PUPDR &= ~((3UL << (8U * 2U)) | (3UL << (9U * 2U)));
    GPIOB_PUPDR |=   (1UL << (8U * 2U)) | (1UL << (9U * 2U));
    /* AFRH: PB8=AF4, PB9=AF4 */
    GPIOB_AFRH &= ~((0xFUL << 0U) | (0xFUL << 4U));
    GPIOB_AFRH |=   (4UL   << 0U) | (4UL   << 4U);
    I2C1_CR1 = 0U; 
    // TIMINGR: APB1=240MHz, Fast Mode 400kHz
    // TODO: MUST CONTROL!!!
    I2C1_TIMINGR = 0x00B03FDFU;
    //Enable
    I2C1_CR1 = I2C_CR1_PE;
}

int8_t i2c_write(uint8_t addr, const uint8_t *data, uint8_t len){
    if (i2c_wait(&I2C1_ISR, I2C_ISR_BUSY, 0) < 0) return -1; // Wait till free
    I2C1_CR2 = I2C_CR2_SADD(addr)   |
               I2C_CR2_NBYTES(len)   |
               I2C_CR2_AUTOEND      |
               I2C_CR2_START;
    for (uint8_t i = 0U; i < len; i++) {
        if (i2c_wait(&I2C1_ISR, I2C_ISR_TXIS, 1) < 0) return -1;
        if (I2C1_ISR & I2C_ISR_NACKF) {
            I2C1_ICR = I2C_ICR_NACKCF;
            return -1;
        }
        I2C1_TXDR = data[i];
    }
    if (i2c_wait(&I2C1_ISR, I2C_ISR_STOPF, 1) < 0) return -1;
    I2C1_ICR = I2C_ICR_STOPCF;
    I2C1_CR2 = 0U;

    return 0;
}

int8_t i2c_read(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t len){
    if (i2c_write(addr, &reg, 1U) < 0) return -1;
    if (i2c_wait(&I2C1_ISR, I2C_ISR_BUSY, 0) < 0) return -1;
    I2C1_CR2 = I2C_CR2_SADD(addr)   |
               I2C_CR2_NBYTES(len)   |
               I2C_CR2_AUTOEND      |
               I2C_CR2_RD_WRN       |
               I2C_CR2_START;
    for (uint8_t i = 0U; i < len; i++) {
        if (i2c_wait(&I2C1_ISR, I2C_ISR_RXNE, 1) < 0) return -1;
        buf[i] = (uint8_t)(I2C1_RXDR & 0xFFU);
    }
    if (i2c_wait(&I2C1_ISR, I2C_ISR_STOPF, 1) < 0) return -1;
    I2C1_ICR = I2C_ICR_STOPCF;
    I2C1_CR2 = 0U;
    return 0;
}
