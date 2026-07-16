/*
 * fault_log.c — STM32H753ZI Backup SRAM fault persistence
 *
 * NOTE ON ADDRESSES: Backup SRAM base, PWR_CR1.DBP, and RCC_AHB4ENR
 * .BKPRAMEN bit position below are my best recollection of RM0433.
 * Same caveat as iwdg.c and fault_handler.c — verify against the
 * reference manual before flashing. Getting the DBP unlock wrong
 * means writes to backup SRAM silently don't take, which is a bad
 * failure mode to discover after a real crash you can't now inspect.
 */

#include "fault_log.h"

/*
 * Cortex-M7 cache maintenance registers (PM0253 / matches the offsets
 * already used in startup_stm32h753zi.s: ICIALLU=0xE000EF50,
 * DCISW=0xE000EF60). DCCMVAC cleans (writes back) a D-cache line
 * containing the given address to physical memory, WITHOUT
 * invalidating it — this is what "clean" means as opposed to
 * "invalidate".
 */
#define DCCMVAC  (*(volatile uint32_t *)0xE000EF68UL)
#define DCACHE_LINE_SIZE  32U   /* Cortex-M7 D-cache line size, bytes */

/*
 * clean_dcache_range
 *   Writes back (but does not discard) every D-cache line overlapping
 *   [addr, addr+size) to physical memory.
 *
 * WHY THIS MATTERS HERE: Reset_Handler in startup_stm32h753zi.s runs
 * "Invalidate D-Cache by set/way" (DCISW) on every boot, BEFORE
 * re-enabling caches. Invalidate discards cache contents — it does
 * NOT write dirty lines back first. If Backup SRAM is cacheable and
 * fault_log_write() only ever updates the cache (write-back policy),
 * the write looks successful in the same run (read-back hits the
 * same cache line) but never actually reaches the physical Backup
 * SRAM cells — so the very next boot's cache invalidate silently
 * throws it away, and the log is gone despite every register/address
 * involved being correct. Cleaning here closes that gap.
 */
static void clean_dcache_range(const void *addr, uint32_t size)
{
    uint32_t start = (uint32_t)addr & ~(DCACHE_LINE_SIZE - 1U);
    uint32_t end   = (uint32_t)addr + size;

    __asm volatile("dsb" ::: "memory");
    while (start < end) {
        DCCMVAC = start;
        start  += DCACHE_LINE_SIZE;
    }
    __asm volatile("dsb" ::: "memory");
    __asm volatile("isb" ::: "memory");
}

/* ── PWR ── */
#define PWR_BASE   0x58024800UL
#define PWR_CR1    (*(volatile uint32_t *)(PWR_BASE + 0x00U))
#define PWR_CR1_DBP  (1UL << 8U)   /* Disable Backup domain write Protection */

/* ── RCC ── */
#define RCC_BASE      0x58024400UL
#define RCC_AHB4ENR   (*(volatile uint32_t *)(RCC_BASE + 0x0E0U))
#define RCC_AHB4ENR_BKPRAMEN  (1UL << 28U)   /* verify bit position vs RM0433 */

/* ── Backup SRAM ── */
#define BKPSRAM_BASE  0x38800000UL   /* 4KB, D3 domain, VBAT-retained */

static volatile fault_log_t * const bkp_log =
    (volatile fault_log_t *)BKPSRAM_BASE;

void fault_log_init(void)
{
    /* Allow CPU writes into the VBAT/backup domain */
    PWR_CR1 |= PWR_CR1_DBP;

    /* Clock the Backup SRAM block itself */
    RCC_AHB4ENR |= RCC_AHB4ENR_BKPRAMEN;
}

void fault_log_write(const fault_log_t *entry)
{
    /* Field-by-field volatile copy — no memcpy dependency in a
       fault path, and no assumption about memcpy being fault-safe
       to call from this context. */
    bkp_log->exc_return  = entry->exc_return;
    bkp_log->pc          = entry->pc;
    bkp_log->lr          = entry->lr;
    bkp_log->xpsr        = entry->xpsr;
    bkp_log->r0          = entry->r0;
    bkp_log->r1          = entry->r1;
    bkp_log->r2          = entry->r2;
    bkp_log->r3          = entry->r3;
    bkp_log->r12         = entry->r12;
    bkp_log->cfsr        = entry->cfsr;
    bkp_log->hfsr        = entry->hfsr;
    bkp_log->mmfar       = entry->mmfar;
    bkp_log->bfar        = entry->bfar;
    bkp_log->mmfar_valid = entry->mmfar_valid;
    bkp_log->bfar_valid  = entry->bfar_valid;

    /* Write magic LAST — it's the "this record is complete and
       valid" flag. If a second fault interrupts this function
       (e.g. we're already in a bad state), a partially-written
       record without the magic set is correctly ignored on next
       boot rather than being read as valid-but-garbled. */
    bkp_log->magic = FAULT_LOG_MAGIC;

    /* Flush the whole record out of D-cache into physical Backup
       SRAM now — see clean_dcache_range() comment above for why
       skipping this silently loses the write on the next reset. */
    clean_dcache_range((const void *)bkp_log, sizeof(fault_log_t));
}

uint8_t fault_log_check_and_clear(fault_log_t *out)
{
    if (bkp_log->magic != FAULT_LOG_MAGIC) {
        return 0U;
    }

    out->exc_return  = bkp_log->exc_return;
    out->pc          = bkp_log->pc;
    out->lr          = bkp_log->lr;
    out->xpsr        = bkp_log->xpsr;
    out->r0          = bkp_log->r0;
    out->r1          = bkp_log->r1;
    out->r2          = bkp_log->r2;
    out->r3          = bkp_log->r3;
    out->r12         = bkp_log->r12;
    out->cfsr        = bkp_log->cfsr;
    out->hfsr        = bkp_log->hfsr;
    out->mmfar       = bkp_log->mmfar;
    out->bfar        = bkp_log->bfar;
    out->mmfar_valid = bkp_log->mmfar_valid;
    out->bfar_valid  = bkp_log->bfar_valid;

    /* Clear magic so this same crash isn't re-reported on the
       next boot too (leave the rest of the fields — harmless,
       and useful for a "last fault regardless of freshness"
       debug read if you ever want to add one). */
    bkp_log->magic = 0U;

    /* Same reasoning as in fault_log_write(): without flushing this
       write out of D-cache, the clear only happens in cache — the
       next boot's cache invalidate silently reverts it, and the same
       fault gets reported again forever, which is exactly what was
       observed (the same PC/LR fault re-appearing after it had
       already been reported and "cleared" on a previous boot). */
    clean_dcache_range((const void *)&bkp_log->magic, sizeof(bkp_log->magic));

    return 1U;
}

uint32_t fault_log_peek_magic(void)
{
    return bkp_log->magic;
}