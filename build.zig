const std = @import("std");

const targets: []const std.Target.Query = &.{
    .{ .os_tag = .windows, .cpu_arch = .x86_64, .abi = .gnu },
    // .{ .os_tag = .windows, .cpu_arch = .aarch64, .abi = .gnu },
    // .{ .os_tag = .linux, .cpu_arch = .x86_64, .abi = .musl },
    // .{ .os_tag = .linux, .cpu_arch = .aarch64, .abi = .musl },
};

pub fn build(b: *std.Build) void {
    for (targets) |t| {
        const exe_mod = b.createModule(.{
            .target = b.resolveTargetQuery(t),
            .optimize = .ReleaseFast,
            .strip = true,
            .link_libc = true,
            .link_libcpp = true,
        });

        exe_mod.addCSourceFiles(.{
            .files = &.{ "src/autoupdate_disable.cpp", "src/cloud_disable.cpp", "src/main.cpp", "src/utility.cpp" },
            .flags = &.{"-std=c++17"},
        });

        exe_mod.addIncludePath(b.path("include"));

        const exe = b.addExecutable(.{
            .name = "Steam NoCloud NoUpdates",
            .root_module = exe_mod,
        });

        const install = b.addInstallArtifact(exe, .{
            .dest_dir = .{
                .override = .{
                    .custom = b.fmt("{s}-{s}", .{ @tagName(t.cpu_arch.?), @tagName(t.os_tag.?) }),
                },
            },
        });

        b.getInstallStep().dependOn(&install.step);
    }
}
