#pragma once
/*#
    # ui_x65.h

    Integrated debugging UI for x65.h

    Do this:
    ~~~C
    #define CHIPS_UI_IMPL
    ~~~
    before you include this file in *one* C++ file to create the
    implementation.

    Optionally provide the following macros with your own implementation

    ~~~C
    CHIPS_ASSERT(c)
    ~~~
        your own assert macro (default: assert(c))

    ## zlib/libpng license

    Copyright (c) 2018 Andre Weissflog
    This software is provided 'as-is', without any express or implied warranty.
    In no event will the authors be held liable for any damages arising from the
    use of this software.
    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:
        1. The origin of this software must not be misrepresented; you must not
        claim that you wrote the original software. If you use this software in a
        product, an acknowledgment in the product documentation would be
        appreciated but is not required.
        2. Altered source versions must be plainly marked as such, and must not
        be misrepresented as being the original software.
        3. This notice may not be removed or altered from any source
        distribution.
#*/
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
    #include "imgui.h"
#endif

#define UI_DASM_USE_M6502
#define UI_DBG_USE_M6502
#include "chips/chips_common.h"
#include "systems/x65.h"
#include "chips/mem.h"
#include "ui/ui_util.h"
#include "ui/ui_chip.h"
#include "ui/ui_memedit.h"
#include "ui/ui_memmap.h"
#include "ui/ui_dasm.h"
#include "ui/ui_dbg.h"
#include "ui/ui_m6502.h"
#include "ui/ui_m6522.h"
#include "ui/ui_m6561.h"
#include "ui/ui_audio.h"
#include "ui/ui_kbd.h"
#include "ui/ui_snapshot.h"

#ifdef __cplusplus
extern "C" {
#endif

// reboot callback
typedef void (*ui_x65_boot_cb)(x65_t* sys);

// setup params for ui_x65_init()
typedef struct {
    x65_t* x65;                              // pointer to x65_t instance to track
    ui_x65_boot_cb boot_cb;                  // reboot callback function
    ui_dbg_texture_callbacks_t dbg_texture;  // user-provided texture create/update/destroy callbacks
    ui_dbg_keys_desc_t dbg_keys;             // user-defined hotkeys for ui_dbg_t
    ui_snapshot_desc_t snapshot;             // snapshot ui setup params
} ui_x65_desc_t;

typedef struct {
    x65_t* x65;
    int dbg_scanline;
    ui_x65_boot_cb boot_cb;
    ui_m6502_t cpu;
    ui_m6522_t via[2];
    ui_m6561_t vic;
    ui_audio_t audio;
    ui_kbd_t kbd;
    ui_memmap_t memmap;
    ui_memedit_t memedit[4];
    ui_dasm_t dasm[4];
    ui_dbg_t dbg;
    ui_snapshot_t snapshot;
    bool system_window_open;
} ui_x65_t;

void ui_x65_init(ui_x65_t* ui, const ui_x65_desc_t* desc);
void ui_x65_discard(ui_x65_t* ui);
void ui_x65_draw(ui_x65_t* ui);
chips_debug_t ui_x65_get_debug(ui_x65_t* ui);

#ifdef __cplusplus
}  // extern "C"
#endif

/*-- IMPLEMENTATION (include in C++ source) ----------------------------------*/
#ifdef CHIPS_UI_IMPL
    #ifndef __cplusplus
        #error "implementation must be compiled as C++"
    #endif
    #include <string.h> /* memset */
    #ifndef CHIPS_ASSERT
        #include <assert.h>
        #define CHIPS_ASSERT(c) assert(c)
    #endif
    #ifdef __clang__
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wmissing-field-initializers"
    #endif

static void _ui_x65_draw_menu(ui_x65_t* ui) {
    CHIPS_ASSERT(ui && ui->x65 && ui->boot_cb);
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("System")) {
            ui_snapshot_menus(&ui->snapshot);
            if (ImGui::MenuItem("Reset")) {
                x65_reset(ui->x65);
                ui_dbg_reset(&ui->dbg);
            }
            if (ImGui::MenuItem("Cold Boot")) {
                ui->boot_cb(ui->x65);
                ui_dbg_reboot(&ui->dbg);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Hardware")) {
            ImGui::MenuItem("System", 0, &ui->system_window_open);
            ImGui::MenuItem("Memory Map", 0, &ui->memmap.open);
            ImGui::MenuItem("Keyboard Matrix", 0, &ui->kbd.open);
            ImGui::MenuItem("Audio Output", 0, &ui->audio.open);
            ImGui::MenuItem("MOS 6502 (CPU)", 0, &ui->cpu.open);
            ImGui::MenuItem("MOS 6522 #1 (VIA)", 0, &ui->via[0].open);
            ImGui::MenuItem("MOS 6522 #2 (VIA)", 0, &ui->via[1].open);
            ImGui::MenuItem("MOS 6561 (VIC-I)", 0, &ui->vic.open);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Debug")) {
            ImGui::MenuItem("CPU Debugger", 0, &ui->dbg.ui.open);
            ImGui::MenuItem("Breakpoints", 0, &ui->dbg.ui.show_breakpoints);
            ImGui::MenuItem("Execution History", 0, &ui->dbg.ui.show_history);
            ImGui::MenuItem("Memory Heatmap", 0, &ui->dbg.ui.show_heatmap);
            if (ImGui::BeginMenu("Memory Editor")) {
                ImGui::MenuItem("Window #1", 0, &ui->memedit[0].open);
                ImGui::MenuItem("Window #2", 0, &ui->memedit[1].open);
                ImGui::MenuItem("Window #3", 0, &ui->memedit[2].open);
                ImGui::MenuItem("Window #4", 0, &ui->memedit[3].open);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Disassembler")) {
                ImGui::MenuItem("Window #1", 0, &ui->dasm[0].open);
                ImGui::MenuItem("Window #2", 0, &ui->dasm[1].open);
                ImGui::MenuItem("Window #3", 0, &ui->dasm[2].open);
                ImGui::MenuItem("Window #4", 0, &ui->dasm[3].open);
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        ui_util_options_menu();
        ImGui::EndMainMenuBar();
    }
}

    // keep disassembler layer at the start
    #define _UI_X65_MEMLAYER_CPU   (0)  // CPU visible mapping
    #define _UI_X65_MEMLAYER_VIC   (1)  // VIC visible mapping
    #define _UI_X65_MEMLAYER_COLOR (2)  // special static color RAM
    #define _UI_X65_CODELAYER_NUM  (1)  // number of valid layers for disassembler
    #define _UI_X65_MEMLAYER_NUM   (3)

static const char* _ui_x65_memlayer_names[_UI_X65_MEMLAYER_NUM] = { "CPU Mapped", "VIC Mapped", "Color RAM" };

static uint8_t _ui_x65_mem_read(int layer, uint16_t addr, void* user_data) {
    CHIPS_ASSERT(user_data);
    ui_x65_t* ui = (ui_x65_t*)user_data;
    x65_t* x65 = ui->x65;
    switch (layer) {
        case _UI_X65_MEMLAYER_CPU: return mem_rd(&x65->mem_cpu, addr);
        case _UI_X65_MEMLAYER_VIC: return mem_rd(&x65->mem_vic, addr);
        case _UI_X65_MEMLAYER_COLOR:
            // static COLOR RAM
            return x65->color_ram[addr & 0x3FF];
        default: return 0xFF;
    }
}

static void _ui_x65_mem_write(int layer, uint16_t addr, uint8_t data, void* user_data) {
    CHIPS_ASSERT(user_data);
    ui_x65_t* ui = (ui_x65_t*)user_data;
    x65_t* x65 = ui->x65;
    switch (layer) {
        case _UI_X65_MEMLAYER_CPU: mem_wr(&x65->mem_cpu, addr, data); break;
        case _UI_X65_MEMLAYER_VIC: mem_wr(&x65->mem_vic, addr, data); break;
        case _UI_X65_MEMLAYER_COLOR:
            // static COLOR RAM
            x65->color_ram[addr & 0x3FF] = data;
            break;
    }
}

static void _ui_x65_update_memmap(ui_x65_t* ui) {
    CHIPS_ASSERT(ui && ui->x65);
    ui_memmap_reset(&ui->memmap);
    ui_memmap_layer(&ui->memmap, "SYS");
    ui_memmap_region(&ui->memmap, "RAM0", 0x0000, 0x0400, true);
    ui_memmap_region(&ui->memmap, "RAM1", 0x1000, 0x1000, true);
    ui_memmap_region(&ui->memmap, "CHAR", 0x8000, 0x1000, true);
    ui_memmap_region(&ui->memmap, "IO", 0x9000, 0x0200, true);
    // FIXME: color ram at variable address
    ui_memmap_region(&ui->memmap, "COLOR", 0x9400, 0x0800, true);
    ui_memmap_region(&ui->memmap, "BASIC", 0xC000, 0x2000, true);
    ui_memmap_region(&ui->memmap, "KERNAL", 0xE000, 0x2000, true);
}

static int _ui_x65_eval_bp(ui_dbg_t* dbg_win, int trap_id, uint64_t pins, void* user_data) {
    (void)pins;
    CHIPS_ASSERT(user_data);
    ui_x65_t* ui = (ui_x65_t*)user_data;
    x65_t* x65 = ui->x65;
    int scanline = x65->vic.rs.v_count;
    for (int i = 0; (i < dbg_win->dbg.num_breakpoints) && (trap_id == 0); i++) {
        const ui_dbg_breakpoint_t* bp = &dbg_win->dbg.breakpoints[i];
        if (bp->enabled) {
            switch (bp->type) {
                // scanline number
                case UI_DBG_BREAKTYPE_USER + 0:
                    if ((ui->dbg_scanline != scanline) && (scanline == bp->val)) {
                        trap_id = UI_DBG_BP_BASE_TRAPID + i;
                    }
                    break;
                // next scanline
                case UI_DBG_BREAKTYPE_USER + 1:
                    if (ui->dbg_scanline != scanline) {
                        trap_id = UI_DBG_BP_BASE_TRAPID + i;
                    }
                    break;
                // next frame
                case UI_DBG_BREAKTYPE_USER + 2:
                    if ((ui->dbg_scanline != scanline) && (scanline == 0)) {
                        trap_id = UI_DBG_BP_BASE_TRAPID + i;
                    }
                    break;
            }
        }
    }
    ui->dbg_scanline = scanline;
    return trap_id;
}

static const ui_chip_pin_t _ui_x65_cpu_pins[] = {
    {"D0",    0,  M6502_D0  },
    { "D1",   1,  M6502_D1  },
    { "D2",   2,  M6502_D2  },
    { "D3",   3,  M6502_D3  },
    { "D4",   4,  M6502_D4  },
    { "D5",   5,  M6502_D5  },
    { "D6",   6,  M6502_D6  },
    { "D7",   7,  M6502_D7  },
    { "RW",   9,  M6502_RW  },
    { "SYNC", 10, M6502_SYNC},
    { "RDY",  11, M6502_RDY },
    { "IRQ",  12, M6502_IRQ },
    { "NMI",  13, M6502_NMI },
    { "RES",  14, M6502_RES },
    { "A0",   16, M6502_A0  },
    { "A1",   17, M6502_A1  },
    { "A2",   18, M6502_A2  },
    { "A3",   19, M6502_A3  },
    { "A4",   20, M6502_A4  },
    { "A5",   21, M6502_A5  },
    { "A6",   22, M6502_A6  },
    { "A7",   23, M6502_A7  },
    { "A8",   24, M6502_A8  },
    { "A9",   25, M6502_A9  },
    { "A10",  26, M6502_A10 },
    { "A11",  27, M6502_A11 },
    { "A12",  28, M6502_A12 },
    { "A13",  29, M6502_A13 },
    { "A14",  30, M6502_A14 },
    { "A15",  31, M6502_A15 },
};

// FIXME
static const ui_chip_pin_t _ui_x65_via_pins[] = {
    {"D0",   0,  M6522_D0 },
    { "D1",  1,  M6522_D1 },
    { "D2",  2,  M6522_D2 },
    { "D3",  3,  M6522_D3 },
    { "D4",  4,  M6522_D4 },
    { "D5",  5,  M6522_D5 },
    { "D6",  6,  M6522_D6 },
    { "D7",  7,  M6522_D7 },
    { "RS0", 9,  M6522_RS0},
    { "RS1", 10, M6522_RS1},
    { "RS2", 11, M6522_RS2},
    { "RS3", 12, M6522_RS3},
    { "RW",  14, M6522_RW },
    { "CS1", 15, M6522_CS1},
    { "CS2", 16, M6522_CS2},
    { "IRQ", 17, M6522_IRQ},
    { "PA0", 20, M6522_PA0},
    { "PA1", 21, M6522_PA1},
    { "PA2", 22, M6522_PA2},
    { "PA3", 23, M6522_PA3},
    { "PA4", 24, M6522_PA4},
    { "PA5", 25, M6522_PA5},
    { "PA6", 26, M6522_PA6},
    { "PA7", 27, M6522_PA7},
    { "CA1", 28, M6522_CA1},
    { "CA2", 29, M6522_CA2},
    { "PB0", 30, M6522_PB0},
    { "PB1", 31, M6522_PB1},
    { "PB2", 32, M6522_PB2},
    { "PB3", 33, M6522_PB3},
    { "PB4", 34, M6522_PB4},
    { "PB5", 35, M6522_PB5},
    { "PB6", 36, M6522_PB6},
    { "PB7", 37, M6522_PB7},
    { "CB1", 38, M6522_CB1},
    { "CB2", 39, M6522_CB2},
};

static const ui_chip_pin_t _ui_x65_vic_pins[] = {
    {"DB0",  0,  M6561_D0 },
    { "DB1", 1,  M6561_D1 },
    { "DB2", 2,  M6561_D2 },
    { "DB3", 3,  M6561_D3 },
    { "DB4", 4,  M6561_D4 },
    { "DB5", 5,  M6561_D5 },
    { "DB6", 6,  M6561_D6 },
    { "DB7", 7,  M6561_D7 },
    { "RW",  9,  M6561_RW },
    { "A0",  14, M6561_A0 },
    { "A1",  15, M6561_A1 },
    { "A2",  16, M6561_A2 },
    { "A3",  17, M6561_A3 },
    { "A4",  18, M6561_A4 },
    { "A5",  19, M6561_A5 },
    { "A6",  20, M6561_A6 },
    { "A7",  21, M6561_A7 },
    { "A8",  22, M6561_A8 },
    { "A9",  23, M6561_A9 },
    { "A10", 24, M6561_A10},
    { "A11", 25, M6561_A11},
    { "A12", 26, M6561_A12},
    { "A13", 27, M6561_A13}
};

void ui_x65_init(ui_x65_t* ui, const ui_x65_desc_t* ui_desc) {
    CHIPS_ASSERT(ui && ui_desc);
    CHIPS_ASSERT(ui_desc->x65);
    CHIPS_ASSERT(ui_desc->boot_cb);
    ui->x65 = ui_desc->x65;
    ui->boot_cb = ui_desc->boot_cb;
    ui_snapshot_init(&ui->snapshot, &ui_desc->snapshot);
    int x = 20, y = 20, dx = 10, dy = 10;
    {
        ui_dbg_desc_t desc = { 0 };
        desc.title = "CPU Debugger";
        desc.x = x;
        desc.y = y;
        desc.m6502 = &ui->x65->cpu;
        desc.read_cb = _ui_x65_mem_read;
        desc.break_cb = _ui_x65_eval_bp;
        desc.texture_cbs = ui_desc->dbg_texture;
        desc.keys = ui_desc->dbg_keys;
        desc.user_data = ui;
        /* custom breakpoint types */
        desc.user_breaktypes[0].label = "Scanline at";
        desc.user_breaktypes[0].show_val16 = true;
        desc.user_breaktypes[1].label = "Next Scanline";
        desc.user_breaktypes[2].label = "Next Frame";
        ui_dbg_init(&ui->dbg, &desc);
    }
    x += dx;
    y += dy;
    {
        ui_m6502_desc_t desc = { 0 };
        desc.title = "MOS 6502";
        desc.cpu = &ui->x65->cpu;
        desc.x = x;
        desc.y = y;
        desc.h = 390;
        UI_CHIP_INIT_DESC(&desc.chip_desc, "6502", 32, _ui_x65_cpu_pins);
        ui_m6502_init(&ui->cpu, &desc);
    }
    x += dx;
    y += dy;
    {
        ui_m6522_desc_t desc = { 0 };
        desc.title = "MOS 6522 #1 (VIA)";
        desc.via = &ui->x65->via_1;
        desc.regs_base = 0x9110;
        desc.x = x;
        desc.y = y;
        UI_CHIP_INIT_DESC(&desc.chip_desc, "6522", 40, _ui_x65_via_pins);
        ui_m6522_init(&ui->via[0], &desc);
        x += dx;
        y += dy;
        desc.title = "MOS 6522 #2 (VIA)";
        desc.via = &ui->x65->via_2;
        desc.regs_base = 0x9120;
        desc.x = x;
        desc.y = y;
        ui_m6522_init(&ui->via[1], &desc);
    }
    x += dx;
    y += dy;
    {
        ui_m6561_desc_t desc = { 0 };
        desc.title = "MOS 6561 (VIC-I)";
        desc.vic = &ui->x65->vic;
        desc.regs_base = 0x9000;
        desc.x = x;
        desc.y = y;
        UI_CHIP_INIT_DESC(&desc.chip_desc, "6561", 28, _ui_x65_vic_pins);
        ui_m6561_init(&ui->vic, &desc);
    }
    x += dx;
    y += dy;
    {
        ui_audio_desc_t desc = { 0 };
        desc.title = "Audio Output";
        desc.sample_buffer = ui->x65->audio.sample_buffer;
        desc.num_samples = ui->x65->audio.num_samples;
        desc.x = x;
        desc.y = y;
        ui_audio_init(&ui->audio, &desc);
    }
    x += dx;
    y += dy;
    {
        ui_kbd_desc_t desc = { 0 };
        desc.title = "Keyboard Matrix";
        desc.kbd = &ui->x65->kbd;
        desc.layers[0] = "None";
        desc.layers[1] = "Shift";
        desc.layers[2] = "Ctrl";
        desc.x = x;
        desc.y = y;
        ui_kbd_init(&ui->kbd, &desc);
    }
    x += dx;
    y += dy;
    {
        ui_memedit_desc_t desc = { 0 };
        for (int i = 0; i < _UI_X65_MEMLAYER_NUM; i++) {
            desc.layers[i] = _ui_x65_memlayer_names[i];
        }
        desc.read_cb = _ui_x65_mem_read;
        desc.write_cb = _ui_x65_mem_write;
        desc.user_data = ui;
        static const char* titles[] = { "Memory Editor #1",
                                        "Memory Editor #2",
                                        "Memory Editor #3",
                                        "Memory Editor #4" };
        for (int i = 0; i < 4; i++) {
            desc.title = titles[i];
            desc.x = x;
            desc.y = y;
            ui_memedit_init(&ui->memedit[i], &desc);
            x += dx;
            y += dy;
        }
    }
    x += dx;
    y += dy;
    {
        ui_memmap_desc_t desc = { 0 };
        desc.title = "Memory Map";
        desc.x = x;
        desc.y = y;
        ui_memmap_init(&ui->memmap, &desc);
    }
    x += dx;
    y += dy;
    {
        ui_dasm_desc_t desc = { 0 };
        for (int i = 0; i < _UI_X65_CODELAYER_NUM; i++) {
            desc.layers[i] = _ui_x65_memlayer_names[i];
        }
        desc.cpu_type = UI_DASM_CPUTYPE_M6502;
        desc.start_addr = mem_rd16(&ui->x65->mem_cpu, 0xFFFC);
        desc.read_cb = _ui_x65_mem_read;
        desc.user_data = ui;
        static const char* titles[4] = { "Disassembler #1", "Disassembler #2", "Disassembler #2", "Dissassembler #3" };
        for (int i = 0; i < 4; i++) {
            desc.title = titles[i];
            desc.x = x;
            desc.y = y;
            ui_dasm_init(&ui->dasm[i], &desc);
            x += dx;
            y += dy;
        }
    }
}

void ui_x65_discard(ui_x65_t* ui) {
    CHIPS_ASSERT(ui && ui->x65);
    ui->x65 = 0;
    ui_m6502_discard(&ui->cpu);
    ui_m6522_discard(&ui->via[0]);
    ui_m6522_discard(&ui->via[1]);
    ui_m6561_discard(&ui->vic);
    ui_kbd_discard(&ui->kbd);
    ui_audio_discard(&ui->audio);
    ui_memmap_discard(&ui->memmap);
    for (int i = 0; i < 4; i++) {
        ui_memedit_discard(&ui->memedit[i]);
        ui_dasm_discard(&ui->dasm[i]);
    }
    ui_dbg_discard(&ui->dbg);
}

void ui_x65_draw_system(ui_x65_t* ui) {
    if (!ui->system_window_open) {
        return;
    }
    x65_t* sys = ui->x65;
    ImGui::SetNextWindowSize({ 200, 250 }, ImGuiCond_Once);
    if (ImGui::Begin("VIC-20 System", &ui->system_window_open)) {}
    ImGui::End();
}

void ui_x65_draw(ui_x65_t* ui) {
    CHIPS_ASSERT(ui && ui->x65);
    _ui_x65_draw_menu(ui);
    if (ui->memmap.open) {
        _ui_x65_update_memmap(ui);
    }
    ui_x65_draw_system(ui);
    ui_audio_draw(&ui->audio, ui->x65->audio.sample_pos);
    ui_kbd_draw(&ui->kbd);
    ui_m6502_draw(&ui->cpu);
    ui_m6522_draw(&ui->via[0]);
    ui_m6522_draw(&ui->via[1]);
    ui_m6561_draw(&ui->vic);
    ui_memmap_draw(&ui->memmap);
    for (int i = 0; i < 4; i++) {
        ui_memedit_draw(&ui->memedit[i]);
        ui_dasm_draw(&ui->dasm[i]);
    }
    ui_dbg_draw(&ui->dbg);
}

chips_debug_t ui_x65_get_debug(ui_x65_t* ui) {
    chips_debug_t res = {};
    res.callback.func = (chips_debug_func_t)ui_dbg_tick;
    res.callback.user_data = &ui->dbg;
    res.stopped = &ui->dbg.dbg.stopped;
    return res;
}

    #ifdef __clang__
        #pragma clang diagnostic pop
    #endif
#endif /* CHIPS_UI_IMPL */
