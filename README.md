# TamgaOS (Yula)

Bare-metal RTOS written in C and ARM assembly.
Started as a learning project — now focused on deterministic scheduling, memory protection, and debuggability.

## Kernel core (shared across all ARM boards)

**Kernel**
- Preemptive scheduler, PendSV context switch, PSP per-task isolation
- FPU context switching (lazy stacking via EXC_RETURN; FPU variant differs per board — see below)

**Sync**
- Mutex (LDREX/STREX) with priority inheritance — elevates the lock owner's priority to match a higher-priority blocked waiter, preventing priority inversion; restores original priority on unlock
- Semaphore with priority-based wait queues
- Critical section (cpsid/cpsie)

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
- Critical section (cpsid/cpsie)

**Drivers**
- SysTick (1ms tick, AHB/8)
- RCC (HSI 64MHz and HSE/PLL1 480MHz)
- UART (USART3, 115200, Virtual COM via ST-Link)
- I2C (I2C1 PB8/PB9 AF4)
- CAN (PA11=RX (AF9), PA12=TX (AF9))
- IWDG (Independent Watchdog, LSI-clocked, software-configurable timeout)

**Sensors**
- MPU6050

**Tests**
- `tests/test_mutex_priority_inheritance.c` - validates elevate/restore behavior under LOW/HIGH/MED priority contention
- `tests/imu_kalman_fdcan.c` - imu kalman fdcan test
- `tests/test_fdcan1.c` - Standart loopback data going/coming test
- `tests/test_fault_handler.c` - deliberately triggers UsageFault/BusFault/MemManage/HardFault and verifies UART dump + Backup SRAM persistence across a real reset
- `tests/test_mpu_stack_guard.c` - runs a real scheduled task to destruction to verify the MPU stack-overflow guard fires with the correct faulting address
- `tests/test_iwdg.c` - arms IWDG, proves kicking prevents reset, then deliberately starves it to confirm the board actually resets and the cause is correctly reported

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

**Sync**
- Mutex (LDREX/STREX) with priority inheritance — elevates the lock owner's priority to match a higher-priority blocked waiter, preventing priority inversion; restores original priority on unlock
- Semaphore with priority-based wait queues
- Critical section (cpsid/cpsie)

**Drivers**
- PIT timer (32-bit, scheduler tick)
- UART
- MCG

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