#ifndef IWDG_H
#define IWDG_H

#include <stdint.h>

/*
 * iwdg_init
 *   Configures the Independent Watchdog with a timeout in milliseconds.
 *   LSI clock is ~32kHz (uncalibrated, can vary +/-10% per datasheet —
 *   do not rely on this for tight timing, only as a fail-safe deadline).
 *
 *   timeout_ms: desired watchdog timeout, clamped to hardware range
 *               (~0.1ms to ~32768ms with prescaler 256 / reload 4095)
 */
void iwdg_init(uint32_t timeout_ms);

/*
 * iwdg_kick
 *   Reloads the watchdog counter. Must be called periodically from
 *   the scheduler tick (or a dedicated low-priority housekeeping task)
 *   at an interval shorter than timeout_ms, or the MCU resets.
 */
void iwdg_kick(void);

/*
 * iwdg_reset_was_watchdog
 *   Returns 1 if the last reset was caused by the IWDG (i.e. a task
 *   hung / missed its kick deadline), 0 for a normal reset cause.
 *   Reads RCC_RSR (IWDGRSTF) — call once early in main() before
 *   clearing reset flags, so you can log/report the fault reason.
 */
uint8_t iwdg_reset_was_watchdog(void);

/*
 * iwdg_clear_reset_flags
 *   Clears the reset cause flags in RCC_RSR. Call after reading
 *   iwdg_reset_was_watchdog() so the next reset cause is fresh.
 */
void iwdg_clear_reset_flags(void);

#endif /* IWDG_H */
