# TamgaOS (Yula)

Bare-metal RTOS written in C and ARM assembly.  
Started as a learning project — now focused on deterministic scheduling, memory protection, and debuggability.

![TamgaOS CAN](images/tamga_can_com.gif)  
*STM32H753ZI (FDCAN) ↔ FRDM-K64F (FlexCAN) — real two-node bus, SN65HVD230 transceivers, verified ACK on both sides*

## Kernel core (shared across all ARM boards)

**Kernel**
- Preemptive scheduler, PendSV context switch, PSP per-task isolation
- FPU context switching (lazy stacking via EXC_RETURN; FPU variant differs per board — see below)

**Sync**
- Mutex (LDREX/STREX) with priority inheritance — elevates the lock owner's priority to match a higher-priority blocked waiter, preventing priority inversion; restores original priority on unlock
- Semaphore with priority-based wait queues
- Queue — fixed-size circular buffer, priority-ordered wait list on both the send side and the receive side (a task blocked on a full/empty queue is served in priority order, not FIFO order)
- Event flags (event groups) — bitmask-based `EVENT_WAIT_ANY` / `EVENT_WAIT_ALL` waiting with optional auto-clear on wake
- Timeout support across the board — `mutex_lock_timeout`, `queue_send_timeout`, `queue_receive_timeout`, `event_wait_timeout` all bound their wait on an absolute deadline (via `systick_get_ms()`), so a task that wakes and loses a race re-arms only the *remaining* budget instead of the full timeout again
- Critical section (cpsid/cpsie)

All four primitives above live in `kernel/core/` and are genuinely board-agnostic — same source file, same behavior, verified independently on both STM32H753ZI and K64F (priority-order, ANY/ALL/auto_clear, and both timeout-expiry/timeout-success paths all pass identically on both ports).

## Actuators (shared abstraction, board-specific PWM backend)

**Servo/ESC**
- `actuators/servo.c` — generic hobby servo/ESC abstraction over each board's PWM driver. Maps a 0-180 degree angle (or raw microsecond pulse, for ESC throttle) to the standard 1000-2000us hobby servo/ESC convention (1000=0deg/min-throttle, 1500=90deg/center, 2000=180deg/max-throttle)
- Continuous 0↔180 sweep helper (`servo_sweep_step()`), independent per channel — verified on both boards with a logic analyzer showing a clean, correctly-timed 20ms/50Hz signal

## ARM port — STM32H753ZI (Cortex-M7)

![TamgaOS STM32H7](images/stm32_tamga.gif)

**Boot**
- Hand-written startup with I-Cache/D-Cache invalidate + enable sequence (PM0253)
- Linker script (DTCM, AXI, SRAM1-4, BKPSRAM)
- RCC init — HSI 64MHz and HSE/PLL1 480MHz

**Kernel**
- Preemptive scheduler (Cortex-M7 port)
- PendSV context switch
- PSP per-task isolation
- FPU context switching (fpv5-d16, double-precision, lazy stacking via EXC_RETURN)
- MPU-based stack overflow guard (hardware-enforced, MemManage on violation — verified end-to-end: a real stack overflow inside a running task trips the guard and lands in the fault handler with the correct MMFAR)

**Fault handling & safety**
- Unified fault handler for HardFault, MemManage, BusFault, and UsageFault — all four vectors route through one handler that dumps PC/LR/xPSR/R0-R3/R12/SP, decodes every CFSR bit (DIVBYZERO, DACCVIOL, IACCVIOL, STKERR, IBUSERR, etc.), and reports MMFAR/BFAR when valid
- Crash record persisted to Backup SRAM (survives a normal reset, not just visible over UART at the moment of the crash) and automatically re-reported on the next boot, then cleared — both the write and the clear are D-cache–flushed explicitly, since a plain cache invalidate-without-clean on the next boot would otherwise silently discard an unflushed write (or an unflushed clear, causing the same crash to be re-reported forever)
- Independent Watchdog (IWDG) — verified to actually reset the board when a task stops kicking it, with the reset cause (`IWDGRSTF` in `RCC_RSR`) correctly distinguished from a normal reset on the next boot
- `FAULT_HANDLER_DEBUG_HALT` build flag: defaults to 0 (production — logs then hangs safely for inspection; pair with IWDG for auto-recovery), set to 1 only on an explicit debug build to halt instead, for live GDB inspection

**Sync**
- Mutex (LDREX/STREX) with priority inheritance — elevates the lock owner's priority to match a higher-priority blocked waiter, preventing priority inversion; restores original priority on unlock
- Semaphore with priority-based wait queues
- Queue — priority-ordered wait on send and receive, with a bounded (`_timeout`) variant
- Event flags — ANY/ALL/auto_clear, with a bounded (`_timeout`) variant
- Critical section (cpsid/cpsie)

**Drivers**
- SysTick (1ms tick, AHB/8) — also the shared kernel's monotonic time source (`systick_get_ms()`) used by every `_timeout` primitive above
- RCC (HSI 64MHz and HSE/PLL1 480MHz)
- UART (USART3, 115200, Virtual COM via ST-Link)
- I2C (I2C1 PB8/PB9 AF4)
- FDCAN1 (PB8=RX (AF9), PB9=TX (AF9) — PA11/PA12 they're tied to USB OTG FS) — verified on a real two-node bus: internal loopback passes, and with an SN65HVD230 transceiver on each board, STM32 and K64F exchange frames continuously with zero ACK failures on either side
- IWDG (Independent Watchdog, LSI-clocked, software-configurable timeout)
- PWM (TIM2_CH1, PA0 / Arduino D32, 50Hz/1-2ms hobby servo & ESC convention) — verified with a logic analyzer: clean 20.0ms period, pulse width tracking the requested 1000-2000us value exactly.  

**Sensors**
- MPU6050

**Tests**
- `tests/test_mutex_priority_inheritance.c` — validates elevate/restore behavior under LOW/HIGH/MED priority contention
- `tests/test_queue_priority_order.c` — validates priority-ordered wake on both the send side and the receive side of a queue
- `tests/test_event_flags.c` — validates ANY/ALL wake, auto_clear, and both timeout-expiry/timeout-success paths
- `tests/test_timeout.c` — validates `mutex_lock_timeout`/`queue_send_timeout`/`queue_receive_timeout` against both an expiring wait and a wait satisfied just before the deadline
- `tests/imu_kalman_fdcan.c` — imu kalman fdcan test
- `tests/test_fdcan1.c` — internal loopback: send/receive round-trip, byte-for-byte
- `tests/stm/test_fdcan_real_bus.c` — real-bus test (loopback disabled): periodic TX + continuous RX polling, paired with the matching K64F test below
- `tests/test_fault_handler.c` — deliberately triggers UsageFault/BusFault/MemManage/HardFault and verifies UART dump + Backup SRAM persistence across a real reset
- `tests/test_mpu_stack_guard.c` — runs a real scheduled task to destruction to verify the MPU stack-overflow guard fires with the correct faulting address
- `tests/test_iwdg.c` — arms IWDG, proves kicking prevents reset, then deliberately starves it to confirm the board actually resets and the cause is correctly reported
- `tests/stm/pwm_test.c` — sweeps TIM2_CH1 pulse width 1000↔2000us continuously, printing each step over UART
- `tests/stm/servo_sweep_test.c` — continuous 0↔180 degree servo sweep via the actuators/servo.c abstraction

Still improving — development notes at https://auctra.app

---

## ARM port — K64F (Cortex-M4)

![TamgaOS K64F](images/tamgaos_rtos.gif)

**Boot**
- Hand-written startup
- Linker script
- MCG clock init (120MHz)

**Kernel**
- Preemptive scheduler
- PendSV context switch
- PSP per-task isolation
- FPU context switching (fpv4-sp-d16, single-precision, lazy stacking via EXC_RETURN)
- Stack overflow guard: software canary only (K64F MPU registers use a non-standard memory layout vs. the STM32H753ZI port; hardware guard planned for a future update)
- Tick source is selectable at build time: **native ARM SysTick by default** (Cortex-M4 shares the same `0xE000E010` SysTick block as the STM32H753ZI port's Cortex-M7 — identical technique), or `TICK_SOURCE=pit` to drive the scheduler tick from the PIT peripheral instead. Either way, board-agnostic kernel code always sees the same `systick_init()`/`systick_get_ms()`.

**Sync**
- Mutex (LDREX/STREX) with priority inheritance — elevates the lock owner's priority to match a higher-priority blocked waiter, preventing priority inversion; restores original priority on unlock
- Semaphore with priority-based wait queues
- Queue — priority-ordered wait on send and receive, with a bounded (`_timeout`) variant
- Event flags — ANY/ALL/auto_clear, with a bounded (`_timeout`) variant
- Critical section (cpsid/cpsie)

**Drivers**
- SysTick (default tick source — see Kernel above) or PIT (32-bit, opt-in via `TICK_SOURCE=pit`)
- UART
- MCG
- FlexCAN0 (PTB18=TX (ALT2), PTB19=RX (ALT2)) — verified on a real two-node bus: internal loopback passes (5 sequential frames, no drops), and with an SN65HVD230 transceiver, K64F and STM32 exchange frames continuously with zero ACK failures on either side.
- PWM (FTM0 + FTM3, 4 channels: PTC1=FTM0_CH0/ALT4, PTC5=FTM0_CH2/ALT7, PTC8=FTM3_CH4/ALT3, PTC9=FTM3_CH5/ALT3 — 50Hz/1-2ms hobby servo & ESC convention) — verified on all 4 channels with a logic analyzer.  

**Tests**
- `tests/k64f/test_queue_priority_order.c` — K64F port of the queue priority-order test
- `tests/k64f/test_event_flags.c` — K64F port of the event-flags test (all 5 scenarios)
- `tests/k64f/test_flexcan_loopback.c` — internal loopback: single frame + 5 sequential frames, plus a non-blocking `rx_pending()` check
- `tests/k64f/test_flexcan_real_bus.c` — real-bus test (loopback disabled), paired with the STM32 test above
- `tests/k64f/pwm_test.c` — sweeps FTM0_CH0 (PTC1, Motor 1) pulse width 1000↔2000us, printing each step over UART
- `tests/k64f/servo_sweep_test.c` — continuous 0↔180 degree sweep on all 4 channels simultaneously (PTC1/PTC5/PTC8/PTC9), each channel tracking independent sweep state  

---

## x86 port — Zig & C kernel

Experimental dual implementation for comparing low-level Zig vs C. Boots via Limine with a Multiboot2 header. **Active development paused on x86 PORT — focus is on ARM.**

**Implemented**
- GDT setup
- Serial monitor
- ISO via xorriso
- Zig + C comparison

**Boot output**
```text
Zig -> TamgaOS
       KERNEL OK
C   -> TamgaOS __C__
       GDT OK __C__
       Kernel OK __C__
```

![TamgaOS x86](images/tamgaos.gif)

---

## Build

### ARM — K64F
```sh
make clean
make BOARD=k64f
make flash BOARD=k64f
```

Use the PIT-based tick source instead of native SysTick:
```sh
make clean BOARD=k64f
make BOARD=k64f TICK_SOURCE=pit
make flash BOARD=k64f
```

### ARM — STM32H753ZI
```sh
make clean
make BOARD=stm32h753zi
make flash BOARD=stm32h753zi
```

Debug build (halts on fault instead of resetting, for GDB inspection):
```sh
make clean BOARD=stm32h753zi
make BOARD=stm32h753zi DEBUG=1
make flash BOARD=stm32h753zi
```

### x86 — Zig
```powershell
zig build -Doptimize=ReleaseFast
.\mkiso.ps1
qemu-system-i386 -cdrom .\TamgaOS.iso -boot d -serial stdio
```

### x86 — C
```powershell
.\mkiso_c.ps1
qemu-system-i386 -cdrom .\TamgaOS_C.iso -boot d -serial stdio
```

---

Not a production OS. Development log: https://auctra.app