const std = @import("std");

const targets: []const std.Target.Query = &.{
    .{ .cpu_arch = .x86_64, .os_tag = .windows },
    .{ .cpu_arch = .x86_64, .os_tag = .linux, .abi = .gnu },
    .{ .cpu_arch = .aarch64, .os_tag = .linux, .abi = .gnu },
};

pub fn build(b: *std.Build) void {
    for (targets) |t| {
        const exe = b.addExecutable(.{
            .name = "disable-steam-cloud-and-auto-update",
            .root_module = b.createModule(.{
                .target = b.resolveTargetQuery(t),
                .optimize = .ReleaseFast,
                .strip = true,
                .link_libc = true,
                .link_libcpp = true,
            }),
        });

        exe.root_module.addCSourceFiles(.{
            .files = &.{
                "src/api.cpp",
                "src/autoupdate_disable.cpp",
                "src/cloud_disable.cpp",
                "src/main.cpp",
                "src/utility.cpp",
            },
            .flags = &.{"-std=c++17"},
        });

        exe.root_module.addIncludePath(b.path("include"));

        const target_output = b.addInstallArtifact(exe, .{
            .dest_dir = .{
                .override = .{
                    .custom = b.fmt("{s}-{s}", .{
                        @tagName(t.cpu_arch.?),
                        @tagName(t.os_tag.?),
                    }),
                },
            },
        });

        b.getInstallStep().dependOn(&target_output.step);
    }
}
