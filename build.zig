const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.resolveTargetQuery(.{
        .cpu_arch = .x86,
        .os_tag = .freestanding,
        .abi = .none,

        // For gdt
        .cpu_features_sub = std.Target.x86.featureSet(&.{
            .mmx,
            .sse,
            .sse2,
            .sse3,
            .ssse3,
            .sse4_1,
            .sse4_2,
            .avx,
            .avx2,
        }),
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

    c_kernel.root_module.addAssemblyFile(b.path("src/multiboot.s"));
    c_kernel.setLinkerScript(b.path("linker.ld"));
    c_kernel.entry = .{ .symbol_name = "_start" };

    b.installArtifact(c_kernel);
}
