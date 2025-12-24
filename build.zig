const std = @import("std");

pub fn build(b: *std.Build) void {
    // Standard target and optimization options
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    // -------------------------------------
    // Third-party dependencies
    // -------------------------------------
    // GLAD library
    // -------------------------------------
    const glad = b.addStaticLibrary(.{
        .name = "glad",
        .target = target,
        .optimize = optimize,
    });

    glad.addCSourceFile(.{
        .file = b.path("external/glad/src/glad.c"),
        .flags = &.{"-std=c11"},
    });

    glad.addIncludePath(b.path("external/glad/include"));

    glad.installHeadersDirectory(
        b.path("external/glad/include"),
        ".",
        .{ .include_extensions = &.{".h"} },
    );
    glad.linkLibC();
    b.installArtifact(glad);

    // -------------------------------------
    // GLFW library (from package manager)
    // -------------------------------------
    // const glfw_dep = b.dependency("glfw", .{
    //     .target = target,
    //     .optimize = optimize,
    // });
    // const glfw = glfw_dep.artifact("glfw");

    // b.installArtifact(glfw);

    // -------------------------------------
    // Build the Vulkyrie Engine Library.
    // -------------------------------------

    // -------------------------------------
    // Build the Pong Game Executable.
    // -------------------------------------

    // -------------------------------------
    // Build the Asteroids Game Executable.
    // -------------------------------------

    // -------------------------------------
    // Build the Vulky CLI Executable.
    // -------------------------------------
    const vulky_cli_executable = b.addExecutable(.{
        .name = "vulky-cli",
        .target = target,
        .optimize = optimize,
    });

    vulky_cli_executable.addCSourceFile(.{
        .file = b.path("vulky-cli/src/main.cpp"),
        .flags = &.{"-std=c++23"},
    });

    vulky_cli_executable.linkLibCpp();
    b.installArtifact(vulky_cli_executable);

    // // -------------------------------------
    // // Vulkyrie Engine Library
    // // -------------------------------------

    // const engine = b.addStaticLibrary(.{
    //     .name = "engine",
    //     .target = target,
    //     .optimize = optimize,
    // });

    // // Engine source files
    // engine.addCSourceFiles(.{
    //     .files = &.{
    //         "engine/src/main.cpp",
    //         "engine/src/core/console_logger.cpp",
    //         "engine/src/core/file_logger.cpp",
    //         "engine/src/core/shader.cpp",
    //         "engine/src/platform/application_manager.cpp",
    //         "engine/src/platform/generic_platform.cpp",
    //         "engine/include/events/event_manager.cpp",
    //     },
    //     .flags = &.{
    //         "-std=c++23",
    //         "-DVULKYRIE_EXPORTS",
    //         "-DVULKYRIE_DEBUG",
    //     },
    // });

    // engine.addIncludePath(b.path("engine/include"));
    // engine.addIncludePath(b.path("engine/src"));
    // engine.installHeadersDirectory(
    //     b.path("engine/include"),
    //     ".",
    //     .{ .include_extensions = &.{".h"} },
    // );

    // engine.linkLibrary(glfw);
    // engine.linkLibrary(glad);
    // engine.linkLibCpp();

    // b.installArtifact(engine);

    // // -------------------------------------
    // // Game: Pong
    // // -------------------------------------

    // const pong = b.addExecutable(.{
    //     .name = "pong",
    //     .target = target,
    //     .optimize = optimize,
    // });

    // pong.addCSourceFiles(.{
    //     .files = &.{
    //         "games/pong/src/pong_entry.cpp",
    //     },
    //     .flags = &.{"-std=c++23"},
    // });

    // pong.linkLibrary(engine);
    // pong.linkLibCpp();

    // b.installArtifact(pong);

    // // Add run step for pong
    // const run_pong_cmd = b.addRunArtifact(pong);
    // run_pong_cmd.step.dependOn(b.getInstallStep());
    // if (b.args) |args| {
    //     run_pong_cmd.addArgs(args);
    // }

    // const run_pong_step = b.step("run-pong", "Run the Pong game");
    // run_pong_step.dependOn(&run_pong_cmd.step);

    // // -------------------------------------
    // // Game: Asteroids
    // // -------------------------------------

    // const asteroids = b.addExecutable(.{
    //     .name = "asteroids",
    //     .target = target,
    //     .optimize = optimize,
    // });

    // asteroids.addCSourceFiles(.{
    //     .files = &.{
    //         "games/asteroids/src/asteroids_entry.cpp",
    //     },
    //     .flags = &.{"-std=c++23"},
    // });

    // asteroids.linkLibrary(engine);
    // asteroids.linkLibCpp();

    // b.installArtifact(asteroids);

    // // Add run step for asteroids
    // const run_asteroids_cmd = b.addRunArtifact(asteroids);
    // run_asteroids_cmd.step.dependOn(b.getInstallStep());
    // if (b.args) |args| {
    //     run_asteroids_cmd.addArgs(args);
    // }

    // const run_asteroids_step = b.step("run-asteroids", "Run the Asteroids game");
    // run_asteroids_step.dependOn(&run_asteroids_cmd.step);

    // // -------------------------------------
    // // Test step (if you add tests later)
    // // -------------------------------------

    // const test_step = b.step("test", "Run all tests");
    // _ = test_step;
}
