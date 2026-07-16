#ifndef FAULT_LOG_H
#define FAULT_LOG_H

#include <stdint.h>

/*
 * fault_log — persists crash diagnostics across a reset.
 *
 * UART alone is not enough: if nobody is watching the terminal at the
 * exact moment of the fault (unattended test rig, field deployment),
 * the dump is lost the instant the reset lands. Backup SRAM survives
 * a system reset (it's only cleared by a power-on reset / VBAT loss),
 * so we write the same info there before resetting, then read it back
 * on the next boot and re-emit it over UART — or hand it to whatever
 * telemetry/logging path exists.
 */

typedef struct {
    uint32_t magic;        /* FAULT_LOG_MAGIC if a log is present and valid */
    uint32_t exc_return;
    uint32_t pc;
    uint32_t lr;
    uint32_t xpsr;
    uint32_t r0, r1, r2, r3, r12;
    uint32_t cfsr;
    uint32_t hfsr;
    uint32_t mmfar;
    uint32_t bfar;
    uint32_t mmfar_valid;   /* 1 if mmfar field is meaningful */
    uint32_t bfar_valid;    /* 1 if bfar field is meaningful */
} fault_log_t;

#define FAULT_LOG_MAGIC  0x4641554CUL   /* "FAUL" */

/*
 * fault_log_init
 *   Enables Backup SRAM clock + backup domain write access.
 *   Call once, early in main(), before fault_log_check_and_clear().
 */
void fault_log_init(void);

/*
 * fault_log_write
 *   Called from fault_handler_c() right before system_reset().
 *   Writes the full diagnostic record into Backup SRAM.
 */
void fault_log_write(const fault_log_t *entry);

/*
 * fault_log_check_and_clear
 *   Call early in main(), after fault_log_init(). If a valid log is
 *   present (magic matches), copies it into *out, clears the magic
 *   so it isn't re-reported next boot, and returns 1. Returns 0 if
 *   no pending fault log existed (normal boot).
 */
uint8_t fault_log_check_and_clear(fault_log_t *out);

/*
 * fault_log_peek_magic
 *   Diagnostic-only: returns the current raw magic value in Backup
 *   SRAM without clearing it. Use this right after fault_log_write()
 *   to verify the write actually took effect in the SAME run — if it
 *   doesn't read back as FAULT_LOG_MAGIC immediately, the problem is
 *   in the write path itself (DBP/clock enable), not in surviving
 *   the reset. Waiting for a second boot to find that out wastes a
 *   debug cycle.
 */
uint32_t fault_log_peek_magic(void);

#endif /* FAULT_LOG_H */