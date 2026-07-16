# TamgaOS (Yula)

Bare-metal RTOS written in C and ARM assembly.  
Started as a learning project — bootloaders, memory layouts, context switching, executable formats.

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
- MPU-based stack overflow guard (hardware-enforced, MemManage/HardFault on violation)

**Sync**
- Mutex (LDREX/STREX) with priority inheritance — elevates the lock owner's priority to match a higher-priority blocked waiter, preventing priority inversion; restores original priority on unlock
- Semaphore with priority-based wait queues
- Critical section (cpsid/cpsie)

**Drivers**
- SysTick (1ms tick, AHB/8)
- RCC (HSI 64MHz and HSE/PLL1 480MHz)
- UART (USART3, 115200, Virtual COM via ST-Link)
- I2C (I2C1 PB8/PB9 AF4)
- CAN ( PA11=RX (AF9), PA12=TX (AF9))  

**Sensors**
- MPU6050

**Tests**
- `tests/test_mutex_priority_inheritance.c` - validates elevate/restore behavior under LOW/HIGH/MED priority contention
- `tests/imu_kalman_fdcan.c` - imu kalman fdcan test   
- `tests/test_fdcan1.c` - Standart loopback data going/coming test     

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

Experimental dual implementation for comparing low-level Zig vs C. Boots via Limine with a Multiboot2 header. Active development paused — focus is on ARM.

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