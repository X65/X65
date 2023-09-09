const std = @import("std");
const sokol = @import("ext/sokol-zig/build.zig");

const cdb_path = "zig-cache/cdb";

// Although this function looks imperative, note that its job is to
// declaratively construct a build graph that will be executed by an external
// runner.
pub fn build(b: *std.Build) void {
    // Standard target options allows the person running `zig build` to choose
    // what target to build for. Here we do not override the defaults, which
    // means any target is allowed, and the default is native. Other options
    // for restricting supported target set are available.
    const target = b.standardTargetOptions(.{});

    // Standard optimization options allow the person running `zig build` to select
    // between Debug, ReleaseSafe, ReleaseFast, and ReleaseSmall. Here we do not
    // set a preferred release mode, allowing the user to decide how to optimize.
    const optimize = b.standardOptimizeOption(.{});

    const flags = [_][]const u8{
        "-Wall",
        "-Wextra",
        "-Werror=return-type",
        "-gen-cdb-fragment-path",
        cdb_path,
    };

    const cflags = flags ++ [_][]const u8{
        "-std=c99",
        "-Wno-implicit-function-declaration",
        "-Wno-missing-braces",
    };
    const cxxflags = flags ++ [_][]const u8{
        "-std=c++17",
        "-fno-rtti",
    };

    const ui = b.option(bool, "ui", "Build with Dear ImGui") orelse false;
    const options = b.addOptions();
    options.addOption(bool, "ui", ui);

    // sokol-zig
    const sokol_build = sokol.buildSokol(b, target, optimize, .{}, "ext/sokol-zig/");

    const exe = b.addExecutable(.{
        .name = "emu",
        // In this case the main source file is merely a path, however, in more
        // complicated build scripts, this could be a generated file.
        .root_source_file = .{ .path = "emu/main.zig" },
        .target = target,
        .optimize = optimize,
    });

    exe.addOptions("build_options", options);

    exe.addModule("args", b.addModule("args", .{ .source_file = .{
        .path = "ext/zig-args/args.zig",
    } }));

    exe.addAnonymousModule("sokol", .{
        .source_file = .{ .path = "ext/sokol-zig/src/sokol/sokol.zig" },
    });
    exe.linkLibrary(sokol_build);

    exe.linkLibC();
    exe.addIncludePath(.{ .path = "emu" });
    exe.addIncludePath(.{ .path = "." });
    exe.addIncludePath(.{ .path = "ext" });
    exe.addIncludePath(.{ .path = "ext/chips" });
    exe.addIncludePath(.{ .path = "ext/chips-test/examples/common" });
    exe.addIncludePath(.{ .path = "ext/chips-test/examples/roms" });
    exe.addIncludePath(.{ .path = "ext/sokol-zig/src/sokol/c" });
    exe.addIncludePath(.{ .path = "ext/sokol" });
    exe.addIncludePath(.{ .path = "ext/sokol/util" });
    exe.addIncludePath(.{ .path = "ext/imgui" });

    exe.addCSourceFiles(&.{
        "emu/x65.c",
        "emu/sokol.c",
        "ext/chips-test/examples/common/clock.c",
        "ext/chips-test/examples/common/fs.c",
        "ext/chips-test/examples/common/gfx.c",
        "ext/chips-test/examples/common/keybuf.c",
        "ext/chips-test/examples/common/prof.c",
    }, &cflags);

    if (ui) {
        exe.addCSourceFiles(&.{
            "emu/x65-ui.cc",
            "ext/chips-test/examples/common/ui.cc",
            "ext/imgui/imgui.cpp",
            "ext/imgui/imgui_draw.cpp",
            "ext/imgui/imgui_tables.cpp",
            "ext/imgui/imgui_widgets.cpp",
        }, &cxxflags);
        exe.linkLibCpp();
        exe.defineCMacro("SOKOL_GLCORE33", null);
    }

    // This declares intent for the executable to be installed into the
    // standard location when the user invokes the "install" step (the default
    // step when running `zig build`).
    b.installArtifact(exe);

    // This *creates* a Run step in the build graph, to be executed when another
    // step is evaluated that depends on it. The next line below will establish
    // such a dependency.
    const run_cmd = b.addRunArtifact(exe);

    // By making the run step depend on the install step, it will be run from the
    // installation directory rather than directly from within the cache directory.
    // This is not necessary, however, if the application depends on other installed
    // files, this ensures they will be present and in the expected location.
    run_cmd.step.dependOn(b.getInstallStep());

    // This allows the user to pass arguments to the application in the build
    // command itself, like this: `zig build run -- arg1 arg2 etc`
    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    // This creates a build step. It will be visible in the `zig build --help` menu,
    // and can be selected like this: `zig build run`
    // This will evaluate the `run` step rather than the default, which is "install".
    const run_step = b.step("run", "Run the app");
    run_step.dependOn(&run_cmd.step);

    // Creates a step for unit testing. This only builds the test executable
    // but does not run it.
    const unit_tests = b.addTest(.{
        .root_source_file = .{ .path = "src/main.zig" },
        .target = target,
        .optimize = optimize,
    });

    const run_unit_tests = b.addRunArtifact(unit_tests);

    // Similar to creating the run step earlier, this exposes a `test` step to
    // the `zig build --help` menu, providing a way for the user to request
    // running the unit tests.
    const test_step = b.step("test", "Run unit tests");
    test_step.dependOn(&run_unit_tests.step);

    // --- tooling ---

    // const clean_cdb = b.addRemoveDirTree(cdb_path);

    const cdb_step = b.step("cdb", "Create compile_commands.json");
    cdb_step.makeFn = &makeCdb;
    cdb_step.dependOn(&exe.step);
}

fn makeCdb(b: *std.Build.Step, prog_node: *std.Progress.Node) anyerror!void {
    _ = prog_node;

    var cdb_file = try std.fs.cwd().createFile("compile_commands.json", .{ .truncate = true });
    defer cdb_file.close();

    _ = try cdb_file.write("[\n");

    var dir = std.fs.cwd().openIterableDir(cdb_path, .{ .no_follow = true }) catch |err| {
        std.debug.print("compilation database fragments dir `{s}` misssing\n", .{cdb_path});
        return err;
    };

    var walker = try dir.walk(b.dependencies.allocator);
    defer walker.deinit();

    while (try walker.next()) |entry| {
        const ext = std.fs.path.extension(entry.basename);
        if (std.mem.eql(u8, ext, ".json")) {
            var json_file = try entry.dir.openFile(entry.basename, .{});
            defer json_file.close();

            try cdb_file.writeFileAllUnseekable(json_file, .{});
        }
    }

    _ = try cdb_file.write("]\n");
}
