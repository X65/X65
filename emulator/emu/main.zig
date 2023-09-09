const std = @import("std");

const argsParser = @import("args");

const UI = @import("build_options").ui;

const sokol = @import("sokol");
const slog = sokol.log;
const sapp = sokol.app;
const saudio = sokol.audio;
const sdtx = sokol.debugtext;

const emu = @cImport({
    @cInclude("systems/x65.h");
    @cInclude("chips/chips_common.h");
    @cInclude("common.h");
    @cInclude("ui.h");
    @cInclude("systems/ui_x65.h");
});
const roms = @cImport({
    @cInclude("vic20-roms.h");
});

const BORDER_TOP = if (UI) 24 else 8;
const BORDER_LEFT = 8;
const BORDER_RIGHT = 8;
const BORDER_BOTTOM = 16;

const x65_snapshot_t = struct {
    version: u32,
    x65: emu.x65_t,
};

var state: struct {
    x65: emu.x65_t = undefined,
    frame_time_us: u32 = 0,
    ticks: u32 = 0,
    emu_time_ms: f64 = 0,
    ui: if (UI) emu.ui_x65_t else void,
    snapshots: if (UI) [emu.UI_SNAPSHOT_MAX_SLOTS]x65_snapshot_t else void,
} = .{
    .x65 = undefined,
    .frame_time_us = 0,
    .ticks = 0,
    .emu_time_ms = 0,
    .ui = undefined,
    .snapshots = undefined,
};

fn push_audio(samples: [*c]const f32, num_samples: c_int, user_data: ?*anyopaque) callconv(.C) void {
    _ = user_data;
    _ = saudio.push(samples, num_samples);
}

// get x65_desc_t struct
fn x65_desc() emu.x65_desc_t {
    return emu.x65_desc_t{
        .audio = .{
            .callback = .{ .func = push_audio, .user_data = undefined },
            .sample_rate = saudio.sampleRate(),
            .volume = 0.3,
            .num_samples = 0,
        },
        .roms = .{
            .chars = .{ .ptr = &roms.dump_vic20_characters_901460_03_bin, .size = @sizeOf(@TypeOf(roms.dump_vic20_characters_901460_03_bin)) },
            .basic = .{ .ptr = &roms.dump_vic20_basic_901486_01_bin, .size = @sizeOf(@TypeOf(roms.dump_vic20_basic_901486_01_bin)) },
            .kernal = .{ .ptr = &roms.dump_vic20_kernal_901486_07_bin, .size = @sizeOf(@TypeOf(roms.dump_vic20_kernal_901486_07_bin)) },
        },
        .debug = if (UI) emu.ui_x65_get_debug(&state.ui) else .{
            .callback = .{
                .func = null,
                .user_data = null,
            },
            .stopped = undefined,
        },
    };
}

fn app_init() callconv(.C) void {
    const desc = x65_desc();
    emu.x65_init(&state.x65, &desc);
    emu.gfx_init(&emu.gfx_desc_t{
        .draw_extra_cb = if (UI) emu.ui_draw else null,
        .border = .{
            .left = BORDER_LEFT,
            .right = BORDER_RIGHT,
            .top = BORDER_TOP,
            .bottom = BORDER_BOTTOM,
        },
        .display_info = emu.x65_display_info(&state.x65),
        .pixel_aspect = .{ .width = 3, .height = 2 },
    });
    emu.keybuf_init(&(emu.keybuf_desc_t){ .key_delay_frames = 5 });
    emu.clock_init();
    emu.prof_init();
    emu.fs_init();
    saudio.setup(.{
        .logger = .{ .func = slog.func },
    });

    if (UI) {
        emu.ui_init(ui_draw_cb);
        emu.ui_x65_init(&state.ui, &(emu.ui_x65_desc_t){
            .x65 = &state.x65,
            .boot_cb = ui_boot_cb,
            .dbg_texture = .{
                .create_cb = emu.ui_create_texture,
                .update_cb = emu.ui_update_texture,
                .destroy_cb = emu.ui_destroy_texture,
            },
            .snapshot = .{
                .load_cb = ui_load_snapshot,
                .save_cb = ui_save_snapshot,
                .empty_slot_screenshot = .{
                    .texture = emu.ui_shared_empty_snapshot_texture(),
                    .portrait = false,
                },
            },
            .dbg_keys = .{
                .cont = .{ .keycode = emu.simgui_map_keycode(@intFromEnum(sapp.Keycode.F5)), .name = "F5" },
                .stop = .{ .keycode = emu.simgui_map_keycode(@intFromEnum(sapp.Keycode.F5)), .name = "F5" },
                .step_over = .{ .keycode = emu.simgui_map_keycode(@intFromEnum(sapp.Keycode.F6)), .name = "F6" },
                .step_into = .{ .keycode = emu.simgui_map_keycode(@intFromEnum(sapp.Keycode.F7)), .name = "F7" },
                .step_tick = .{ .keycode = emu.simgui_map_keycode(@intFromEnum(sapp.Keycode.F8)), .name = "F8" },
                .toggle_breakpoint = .{ .keycode = emu.simgui_map_keycode(@intFromEnum(sapp.Keycode.F9)), .name = "F9" },
            },
        });
        ui_load_snapshots_from_storage();
    }
}

// per frame stuff, tick the emulator, handle input, decode and draw emulator display
fn app_frame() callconv(.C) void {
    state.frame_time_us = emu.clock_frame_time();
    const emu_start_time = emu.stm_now();
    state.ticks = emu.x65_exec(&state.x65, state.frame_time_us);
    state.emu_time_ms = emu.stm_ms(emu.stm_since(emu_start_time));
    draw_status_bar();
    emu.gfx_draw(emu.x65_display_info(&state.x65));
    // handle_file_loading();
    // send_keybuf_input();
}

fn app_input(event: [*c]const sapp.Event) callconv(.C) void {
    // accept dropped files also when ImGui grabs input
    if (event.*.type == sapp.EventType.FILES_DROPPED) {
        emu.fs_start_load_dropped_file(emu.FS_SLOT_IMAGE);
    }
    if (UI) {
        const touches: [8]emu.sapp_touchpoint = .{};
        const ev: emu.sapp_event = .{
            .frame_count = event.*.frame_count,
            .type = @intCast(@intFromEnum(event.*.type)),
            .key_code = @intCast(@intFromEnum(event.*.key_code)),
            .char_code = event.*.char_code,
            .key_repeat = event.*.key_repeat,
            .modifiers = event.*.modifiers,
            .mouse_button = @intCast(@intFromEnum(event.*.mouse_button)),
            .mouse_x = event.*.mouse_x,
            .mouse_y = event.*.mouse_y,
            .mouse_dx = event.*.mouse_dx,
            .mouse_dy = event.*.mouse_dy,
            .scroll_x = event.*.scroll_x,
            .scroll_y = event.*.scroll_y,
            .num_touches = event.*.num_touches,
            .touches = touches,
            .window_width = event.*.window_width,
            .window_height = event.*.window_height,
            .framebuffer_width = event.*.framebuffer_width,
            .framebuffer_height = event.*.framebuffer_height,
        };
        if (emu.ui_input(&ev)) {
            // input was handled by UI
            return;
        }
    }
    const shift = event.*.modifiers & sapp.modifier_shift;
    switch (event.*.type) {
        sapp.EventType.CHAR => {
            var c: u8 = @intCast(event.*.char_code & 0xFF);
            if ((c > 0x20) and (c < 0x7F)) {
                // need to invert case (unshifted is upper caps, shifted is lower caps
                if (std.ascii.isUpper(c)) {
                    c = std.ascii.toLower(c);
                } else if (std.ascii.isLower(c)) {
                    c = std.ascii.toUpper(c);
                }
                emu.x65_key_down(&state.x65, c);
                emu.x65_key_up(&state.x65, c);
            }
        },
        sapp.EventType.KEY_DOWN, sapp.EventType.KEY_UP => {
            var c: u8 = 0;
            switch (event.*.key_code) {
                sapp.Keycode.SPACE => c = 0x20,
                sapp.Keycode.LEFT => c = 0x08,
                sapp.Keycode.RIGHT => c = 0x09,
                sapp.Keycode.DOWN => c = 0x0A,
                sapp.Keycode.UP => c = 0x0B,
                sapp.Keycode.ENTER => c = 0x0D,
                sapp.Keycode.BACKSPACE => {
                    if (shift != 0) c = 0x0C else c = 0x01;
                },
                sapp.Keycode.ESCAPE => {
                    if (shift != 0) c = 0x13 else c = 0x03;
                },
                sapp.Keycode.F1 => c = 0xF1,
                sapp.Keycode.F2 => c = 0xF2,
                sapp.Keycode.F3 => c = 0xF3,
                sapp.Keycode.F4 => c = 0xF4,
                sapp.Keycode.F5 => c = 0xF5,
                sapp.Keycode.F6 => c = 0xF6,
                sapp.Keycode.F7 => c = 0xF7,
                sapp.Keycode.F8 => c = 0xF8,
                else => {},
            }
            if (c != 0) {
                if (event.*.type == sapp.EventType.KEY_DOWN) {
                    emu.x65_key_down(&state.x65, c);
                } else {
                    emu.x65_key_up(&state.x65, c);
                }
            }
        },
        else => {},
    }
}

fn app_cleanup() callconv(.C) void {
    emu.x65_discard(&state.x65);
    if (UI) {
        emu.ui_x65_discard(&state.ui);
        emu.ui_discard();
    }
    saudio.shutdown();
    emu.gfx_shutdown();
}

fn draw_status_bar() void {
    emu.prof_push(emu.PROF_EMU, @floatCast(state.emu_time_ms));
    const emu_stats = emu.prof_stats(emu.PROF_EMU);
    const w = sapp.widthf();
    const h = sapp.heightf();
    sdtx.canvas(w, h);
    sdtx.color3b(255, 255, 255);
    sdtx.pos(1.0, (h / 8.0) - 1.5);
    sdtx.print("frame:{d:.2}ms emu:{d:.2}ms (min:{d:.2}ms max:{d:.2}ms) ticks:{d}", .{
        @as(f32, @floatFromInt(state.frame_time_us)) * 0.001,
        emu_stats.avg_val,
        emu_stats.min_val,
        emu_stats.max_val,
        state.ticks,
    });
}

fn ui_draw_cb() callconv(.C) void {
    emu.ui_x65_draw(&state.ui);
}

fn ui_boot_cb(sys: [*c]emu.x65_t) callconv(.C) void {
    const desc = x65_desc();
    emu.x65_init(sys, &desc);
}

fn ui_update_snapshot_screenshot(slot: usize) callconv(.C) void {
    const screenshot: emu.ui_snapshot_screenshot_t = .{
        .texture = emu.ui_create_screenshot_texture(emu.x65_display_info(&state.snapshots[slot].x65)),
        .portrait = false,
    };
    const prev_screenshot = emu.ui_snapshot_set_screenshot(&state.ui.snapshot, slot, screenshot);
    if (prev_screenshot.texture != null) {
        emu.ui_destroy_texture(prev_screenshot.texture);
    }
}

fn ui_save_snapshot(slot: usize) callconv(.C) void {
    if (slot < emu.UI_SNAPSHOT_MAX_SLOTS) {
        state.snapshots[slot].version = emu.x65_save_snapshot(&state.x65, &state.snapshots[slot].x65);
        ui_update_snapshot_screenshot(slot);
        _ = emu.fs_save_snapshot("x65", slot, (emu.chips_range_t){ .ptr = &state.snapshots[slot], .size = @sizeOf(x65_snapshot_t) });
    }
}

fn ui_load_snapshot(slot: usize) callconv(.C) bool {
    var success = false;
    if ((slot < emu.UI_SNAPSHOT_MAX_SLOTS) and (state.ui.snapshot.slots[slot].valid)) {
        success = emu.x65_load_snapshot(&state.x65, state.snapshots[slot].version, &state.snapshots[slot].x65);
    }
    return success;
}

fn ui_fetch_snapshot_callback(response: [*c]const emu.fs_snapshot_response_t) callconv(.C) void {
    std.debug.assert(response != null);
    if (response.*.result != emu.FS_RESULT_SUCCESS) {
        return;
    }
    if (response.*.data.size != @sizeOf(x65_snapshot_t)) {
        return;
    }
    const snapshot_ptr: *x65_snapshot_t = @ptrCast(@alignCast(response.*.data.ptr));
    if (snapshot_ptr.*.version != emu.X65_SNAPSHOT_VERSION) {
        return;
    }
    const snapshot_slot = response.*.snapshot_index;
    std.debug.assert(snapshot_slot < emu.UI_SNAPSHOT_MAX_SLOTS);
    const source_ptr: [*]u8 = @ptrCast(response.*.data.ptr);
    const source: []u8 = source_ptr[0..response.*.data.size];
    const dest_ptr: [*]u8 = @ptrCast(&state.snapshots[snapshot_slot]);
    @memcpy(dest_ptr, source);
    ui_update_snapshot_screenshot(snapshot_slot);
}

fn ui_load_snapshots_from_storage() callconv(.C) void {
    for (0..emu.UI_SNAPSHOT_MAX_SLOTS) |snapshot_slot| {
        _ = emu.fs_start_load_snapshot(
            emu.FS_SLOT_SNAPSHOTS,
            "x65",
            snapshot_slot,
            ui_fetch_snapshot_callback,
        );
    }
}

pub fn main() !u8 {
    var argsAllocator = std.heap.page_allocator;

    const Options = struct {
        // This declares long options for double hyphen
        console: bool = false,
        fs: ?[]const u8 = null,
        help: bool = false,

        // This declares short-hand options for single hyphen
        pub const shorthands = .{
            .c = "console",
        };

        pub const meta = .{
            .usage_summary = "[options] [ROM file]",
            .full_text = "X65 system emulator.\nhttp://x65.zone",
            .option_docs = .{
                .console = "connect UART to stdin/out",
                .fs = "internal RIA filesystem directory",
                .help = "Print this help",
            },
        };
    };
    const options = argsParser.parseForCurrentProcess(Options, argsAllocator, .print) catch return 1;
    defer options.deinit();

    if (options.options.help) {
        try argsParser.printHelp(
            Options,
            options.executable_name orelse "emu",
            std.io.getStdOut().writer(),
        );
        return 0;
    }

    const info = emu.x65_display_info(0);
    sapp.run(.{
        .init_cb = app_init,
        .frame_cb = app_frame,
        .event_cb = app_input,
        .cleanup_cb = app_cleanup,
        .width = 3 * info.screen.width + BORDER_LEFT + BORDER_RIGHT,
        .height = 2 * info.screen.height + BORDER_TOP + BORDER_BOTTOM,
        .icon = .{ .sokol_default = true },
        .window_title = "X65",
        .enable_dragndrop = true,
        .logger = .{ .func = slog.func },
    });
    return 0;
}
