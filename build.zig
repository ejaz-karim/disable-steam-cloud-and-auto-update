const std = @import("std");

const release_targets: []const std.Target.Query = &.{
    .{ .cpu_arch = .x86_64, .os_tag = .windows },
    .{ .cpu_arch = .x86_64, .os_tag = .linux, .abi = .musl },
    .{ .cpu_arch = .aarch64, .os_tag = .linux, .abi = .musl },
};

const src_files: []const []const u8 = &.{
    "src/api.cpp",
    "src/autoupdate_disable.cpp",
    "src/cloud_disable.cpp",
    "src/main.cpp",
    "src/utility.cpp",
};

pub fn build(b: *std.Build) void {
    for (release_targets) |t| {
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
            .files = src_files,
            .flags = &.{"-std=c++17"},
        });

        exe.root_module.addIncludePath(b.path("include"));

        const install = b.addInstallArtifact(exe, .{
            .dest_dir = .{
                .override = .{
                    .custom = b.fmt("{s}-{s}", .{
                        @tagName(t.cpu_arch.?),
                        @tagName(t.os_tag.?),
                    }),
                },
            },
        });

        b.getInstallStep().dependOn(&install.step);
    }
}
