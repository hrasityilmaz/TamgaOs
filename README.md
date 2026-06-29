# TamgaOS

Bare-metal RTOS written in C and ARM assembly for the NXP K64F (Cortex-M4).  
Started as a learning project — bootloaders, memory layouts, context switching, executable formats.

Later will add zig port

![TamgaOS](images/tamgaos_rtos.gif)

## ARM port — K64F (Cortex-M4)

Currently active. All kernel work targets the NXP K64F board. Zig port planned for later.

**Boot**
- Startup file 
- Linker script (linker.ld)
- MCG clock init

**Kernel**
- Preemptive scheduler
- PendSV context switch
- PSP per-task isolation

**Sync**
- Mutex (LDREX/STREX)
- Semaphore
- Critical section (cpsid/cpsie)

**Drivers**
- SysTick
- UART
- PIT timer

Still improving — development notes at https://auctra.app

## x86 port — Zig & C kernel

Experimental dual implementation for comparing low-level Zig vs C. Boots via Limine with a Multiboot2 header.

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

![TamgaOS](images/tamgaos.gif)

## Build

### ARM — K64F
```sh
make clean
make 
make flash
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
Allredy x86 part will not continue active development will be on ARM part !

---

Not a production OS. Development log: https://auctra.app