const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.resolveTargetQuery(.{
        .cpu_arch = .x86,
        .os_tag = .freestanding,
        .abi = .none,

        // For gdt
        //.cpu_features_sub = std.Target.x86.featureSet(&.{
        //    .mmx,
        //    .sse,
        //    .sse2,
        //    .sse3,
        // .ssse3,
        //    .sse4_1,
        //    .sse4_2,
        //    .avx,
        //    .avx2,
        //}),
    });

    const optimize = b.standardOptimizeOption(.{});

    // Zig
    const kernel = b.addExecutable(.{
        .name = "kernel",
        .root_module = b.createModule(.{
            .root_source_file = b.path("src/main.zig"),
            .target = target,
            .optimize = optimize,
            .red_zone = false,
            .single_threaded = true,
        }),
    });

    const nasm = b.addSystemCommand(&.{"nasm"});
    nasm.addArgs(&.{ "-f", "elf32", "-o" });
    const gdt_obj = nasm.addOutputFileArg("gdt.o");
    nasm.addFileArg(b.path("src/gdt.asm"));

    const nasm_isr = b.addSystemCommand(&.{"nasm"});
    nasm_isr.addArgs(&.{ "-f", "elf32", "-o" });
    const isr_obj = nasm_isr.addOutputFileArg("isr.o");
    nasm_isr.addFileArg(b.path("src/isr.asm"));

    kernel.root_module.addObjectFile(gdt_obj);
    kernel.root_module.addObjectFile(isr_obj);
    kernel.root_module.addAssemblyFile(b.path("src/multiboot.s"));
    kernel.setLinkerScript(b.path("linker.ld"));
    kernel.entry = .{ .symbol_name = "_start" };

    b.installArtifact(kernel);

    // C
    const c_kernel = b.addExecutable(.{
        .name = "c_kernel",
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .red_zone = false,
            .single_threaded = true,
        }),
    });

    c_kernel.root_module.addCSourceFile(.{
        .file = b.path("src/C/kernel.c"),
        .flags = &.{
            "-ffreestanding",
            "-fno-stack-protector",
            "-fno-builtin",
        },
    });

    c_kernel.root_module.addCSourceFile(.{
        .file = b.path("src/C/gdt.c"),
        .flags = &.{
            "-ffreestanding",
            "-fno-stack-protector",
            "-fno-builtin",
        },
    });

    c_kernel.root_module.addCSourceFile(.{
        .file = b.path("src/C/serial.c"),
        .flags = &.{
            "-ffreestanding",
            "-fno-stack-protector",
            "-fno-builtin",
        },
    });

    c_kernel.root_module.addCSourceFile(.{
        .file = b.path("src/C/isr.c"),
        .flags = &.{
            "-ffreestanding",
            "-fno-stack-protector",
            "-fno-builtin",
        },
    });

    c_kernel.root_module.addCSourceFile(.{
        .file = b.path("src/C/idt.c"),
        .flags = &.{
            "-ffreestanding",
            "-fno-stack-protector",
            "-fno-builtin",
        },
    });

    const nasm_isr_c = b.addSystemCommand(&.{"nasm"});
    nasm_isr_c.addArgs(&.{ "-f", "elf32", "-o" });
    const isr_obj_c = nasm_isr_c.addOutputFileArg("isr_c.o");
    nasm_isr_c.addFileArg(b.path("src/isr.asm"));

    c_kernel.root_module.addObjectFile(isr_obj_c);
    c_kernel.root_module.addAssemblyFile(b.path("src/multiboot.s"));
    c_kernel.setLinkerScript(b.path("linker.ld"));
    c_kernel.entry = .{ .symbol_name = "_start" };

    b.installArtifact(c_kernel);
}
