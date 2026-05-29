const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.resolveTargetQuery(.{
        .cpu_arch = .x86,
        .os_tag = .freestanding,
        .abi = .none,
    });

    const optimize = b.standardOptimizeOption(.{});

    // Zig kernel
    const kernel = b.addExecutable(.{
        .name = "kernel",
        .root_module = b.createModule(.{
            .root_source_file = b.path("src/main.zig"),
            .target = target,
            .optimize = optimize,
        }),
    });

    kernel.root_module.addAssemblyFile(b.path("src/multiboot.s"));
    kernel.setLinkerScript(b.path("linker.ld"));
    kernel.entry = .{ .symbol_name = "_start" };
    kernel.root_module.red_zone = false;
    kernel.root_module.single_threaded = true;

    b.installArtifact(kernel);

    const c_kernel = b.addExecutable(.{
        .name = "c_kernel",
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
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

    c_kernel.root_module.addAssemblyFile(b.path("src/multiboot.s"));
    c_kernel.setLinkerScript(b.path("linker.ld"));
    c_kernel.entry = .{ .symbol_name = "_start" };
    c_kernel.root_module.red_zone = false;
    c_kernel.root_module.single_threaded = true;

    b.installArtifact(c_kernel);
}
