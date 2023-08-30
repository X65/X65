#pragma once
/*#
    # x65.h

    The X65 emulator in a C header.

    Do this:
    ~~~C
    #define CHIPS_IMPL
    ~~~
    before you include this file in *one* C or C++ file to create the
    implementation.

    Optionally provide the following macros with your own implementation

    ~~~C
    CHIPS_ASSERT(c)
    ~~~
        your own assert macro (default: assert(c))

    ## The X65


    TODO!

    ## Links

    http://blog.tynemouthsoftware.co.uk/2019/09/how-the-x65-works.html

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
#include <stddef.h>

#include "chips/chips_common.h"
#include "chips/m6502.h"
#include "chips/m6522.h"
#include "chips/m6561.h"
#include "chips/kbd.h"
#include "chips/mem.h"
#include "chips/clk.h"

#ifdef __cplusplus
extern "C" {
#endif

// bump snapshot version when x65_t memory layout changes
#define X65_SNAPSHOT_VERSION (1)

#define X65_FREQUENCY             (1108404)
#define X65_MAX_AUDIO_SAMPLES     (1024)  // max number of audio samples in internal sample buffer
#define X65_DEFAULT_AUDIO_SAMPLES (128)   // default number of samples in internal sample buffer

// config parameters for x65_init()
typedef struct {
    chips_debug_t debug;  // optional debugging hook
    chips_audio_desc_t audio;
    struct {
        chips_range_t chars;   // 4 KByte character ROM dump
        chips_range_t basic;   // 8 KByte BASIC dump
        chips_range_t kernal;  // 8 KByte KERNAL dump
    } roms;
} x65_desc_t;

// X65 emulator state
typedef struct {
    m6502_t cpu;
    m6522_t via_1;
    m6522_t via_2;
    m6561_t vic;
    uint64_t pins;

    kbd_t kbd;      // keyboard matrix state
    mem_t mem_cpu;  // CPU-visible memory mapping
    mem_t mem_vic;  // VIC-visible memory mapping
    bool valid;
    chips_debug_t debug;

    struct {
        chips_audio_callback_t callback;
        int num_samples;
        int sample_pos;
        float sample_buffer[X65_MAX_AUDIO_SAMPLES];
    } audio;

    uint8_t color_ram[0x0400];   // special color RAM
    uint8_t ram0[0x0400];        // 1 KB zero page, stack, system work area
    uint8_t ram_3k[0x0C00];      // optional 3K exp RAM
    uint8_t ram1[0x1000];        // 4 KB main RAM
    uint8_t rom_char[0x1000];    // 4 KB character ROM image
    uint8_t rom_basic[0x2000];   // 8 KB BASIC ROM image
    uint8_t rom_kernal[0x2000];  // 8 KB KERNAL V3 ROM image
    uint8_t ram_exp[4][0x2000];  // optional expansion 8K RAM blocks
    uint8_t fb[M6561_FRAMEBUFFER_SIZE_BYTES];
} x65_t;

// initialize a new X65 instance
void x65_init(x65_t* sys, const x65_desc_t* desc);
// discard X65 instance
void x65_discard(x65_t* sys);
// reset a X65 instance
void x65_reset(x65_t* sys);
// query display information
chips_display_info_t x65_display_info(x65_t* sys);
// tick X65 instance for a given number of microseconds, return number of executed ticks
uint32_t x65_exec(x65_t* sys, uint32_t micro_seconds);
// send a key-down event to the X65
void x65_key_down(x65_t* sys, int key_code);
// send a key-up event to the X65
void x65_key_up(x65_t* sys, int key_code);
// quickload a .prg/.bin file
bool x65_quickload(x65_t* sys, chips_range_t data);
// load a .prg/.bin file as ROM cartridge
bool x65_insert_rom_cartridge(x65_t* sys, chips_range_t data);
// take a snapshot, patches pointers to zero or offsets, returns snapshot version
uint32_t x65_save_snapshot(x65_t* sys, x65_t* dst);
// load a snapshot, returns false if snapshot version doesn't match
bool x65_load_snapshot(x65_t* sys, uint32_t version, x65_t* src);

#ifdef __cplusplus
}  // extern "C"
#endif

/*-- IMPLEMENTATION ----------------------------------------------------------*/
#ifdef CHIPS_IMPL
    #include <string.h> /* memcpy, memset */
    #ifndef CHIPS_ASSERT
        #include <assert.h>
        #define CHIPS_ASSERT(c) assert(c)
    #endif

    #define _X65_SCREEN_WIDTH  (232)  // actually 229, but rounded up to 8x
    #define _X65_SCREEN_HEIGHT (272)
    #define _X65_SCREEN_X      (32)
    #define _X65_SCREEN_Y      (8)

static uint16_t _x65_vic_fetch(uint16_t addr, void* user_data);
static void _x65_init_key_map(x65_t* sys);

    #define _X65_DEFAULT(val, def) (((val) != 0) ? (val) : (def))

void x65_init(x65_t* sys, const x65_desc_t* desc) {
    CHIPS_ASSERT(sys && desc);
    if (desc->debug.callback.func) {
        CHIPS_ASSERT(desc->debug.stopped);
    }

    memset(sys, 0, sizeof(x65_t));
    sys->valid = true;
    sys->debug = desc->debug;
    sys->audio.callback = desc->audio.callback;
    sys->audio.num_samples = _X65_DEFAULT(desc->audio.num_samples, X65_DEFAULT_AUDIO_SAMPLES);
    CHIPS_ASSERT(sys->audio.num_samples <= X65_MAX_AUDIO_SAMPLES);
    CHIPS_ASSERT(desc->roms.chars.ptr && (desc->roms.chars.size == sizeof(sys->rom_char)));
    CHIPS_ASSERT(desc->roms.basic.ptr && (desc->roms.basic.size == sizeof(sys->rom_basic)));
    CHIPS_ASSERT(desc->roms.kernal.ptr && (desc->roms.kernal.size == sizeof(sys->rom_kernal)));
    memcpy(sys->rom_char, desc->roms.chars.ptr, sizeof(sys->rom_char));
    memcpy(sys->rom_basic, desc->roms.basic.ptr, sizeof(sys->rom_basic));
    memcpy(sys->rom_kernal, desc->roms.kernal.ptr, sizeof(sys->rom_kernal));

    sys->pins = m6502_init(&sys->cpu, &(m6502_desc_t){ 0 });
    m6522_init(&sys->via_1);
    m6522_init(&sys->via_2);
    m6561_init(&sys->vic, &(m6561_desc_t){
        .fetch_cb = _x65_vic_fetch,
        .framebuffer = {
            .ptr = sys->fb,
            .size = sizeof(sys->fb)
        },
        .screen = {
            .x = _X65_SCREEN_X,
            .y = _X65_SCREEN_Y,
            .width = _X65_SCREEN_WIDTH,
            .height = _X65_SCREEN_HEIGHT,
        },
        .user_data = sys,
        .tick_hz = X65_FREQUENCY,
        .sound_hz = _X65_DEFAULT(desc->audio.sample_rate, 44100),
        .sound_magnitude = _X65_DEFAULT(desc->audio.volume, 1.0f),
    });
    _x65_init_key_map(sys);

    /*
        X65 CPU memory map:

        0000..03FF      zero-page, stack, system area
        [0400..0FFF]    3 KB Expansion RAM
        1000..1FFF      4 KB Main RAM (block 0)
        [2000..3FFF]    8 KB Expansion Block 1
        [4000..5FFF]    8 KB Expansion Block 2
        [6000..7FFF]    8 KB Expansion Block 3
        8000..8FFF      4 KB Character ROM
        9000..900F      VIC Registers
        9110..911F      VIA #1 Registers
        9120..912F      VIA #2 Registers
        9400..97FF      1Kx4 bit color ram (either at 9600 or 9400)
        [9800..9BFF]    1 KB I/O Expansion 2
        [9C00..9FFF]    1 KB I/O Expansion 3
        [A000..BFFF]    8 KB Expansion Block 5 (usually ROM cartridges)
        C000..DFFF      8 KB BASIC ROM
        E000..FFFF      8 KB KERNAL ROM

        NOTE: use mem layer 1 for the standard RAM/ROM, so that
        the higher-priority layer 0 can be used for ROM cartridges
    */
    mem_init(&sys->mem_cpu);
    mem_map_ram(&sys->mem_cpu, 1, 0x0000, 0x0400, sys->ram0);
    mem_map_ram(&sys->mem_cpu, 1, 0x1000, 0x1000, sys->ram1);
    mem_map_rom(&sys->mem_cpu, 1, 0x8000, 0x1000, sys->rom_char);
    mem_map_ram(&sys->mem_cpu, 1, 0x9400, 0x0400, sys->color_ram);
    mem_map_rom(&sys->mem_cpu, 1, 0xC000, 0x2000, sys->rom_basic);
    mem_map_rom(&sys->mem_cpu, 1, 0xE000, 0x2000, sys->rom_kernal);

    /*
        VIC-I memory map:

        The VIC-I has 14 address bus bits VA0..VA13, for 16 KB of
        addressable memory. Bits VA0..VA12 are identical with the
        lower 13 CPU address bus pins, VA13 is the inverted BLK4
        address decoding bit.
    */
    mem_init(&sys->mem_vic);
    mem_map_rom(&sys->mem_vic, 0, 0x0000, 0x1000, sys->rom_char);  // CPU: 8000..8FFF
    // FIXME: can the VIC read the color RAM as data?
    // mem_map_rom(&sys->mem_vic, 0, 0x1400, 0x0400, sys->color_ram);      // CPU: 9400..97FF
    mem_map_rom(&sys->mem_vic, 0, 0x2000, 0x0400, sys->ram0);  // CPU: 0000..03FF
    mem_map_rom(&sys->mem_vic, 0, 0x3000, 0x1000, sys->ram1);  // CPU: 1000..1FFF
}

void x65_discard(x65_t* sys) {
    CHIPS_ASSERT(sys && sys->valid);
    sys->valid = false;
}

void x65_reset(x65_t* sys) {
    CHIPS_ASSERT(sys && sys->valid);
    sys->pins |= M6502_RES;
    m6522_reset(&sys->via_1);
    m6522_reset(&sys->via_2);
    m6561_reset(&sys->vic);
}

static uint64_t _x65_tick(x65_t* sys, uint64_t pins) {
    // tick the CPU
    pins = m6502_tick(&sys->cpu, pins);

    // the IRQ and NMI pins will be set by the VIAs each tick
    pins &= ~(M6502_IRQ | M6502_NMI);

    // VIC+VIAs address decoding and memory access
    uint64_t vic_pins = pins & M6502_PIN_MASK;
    uint64_t via1_pins = pins & M6502_PIN_MASK;
    uint64_t via2_pins = pins & M6502_PIN_MASK;
    if ((pins & 0xFC00) == 0x9000) {
        /* 9000..93FF: VIA and VIC IO area

            A4 high:    VIA-1
            A5 high:    VIA-2

            The VIC has no chip-select pin instead it's directly snooping
            the address bus for a specific pin state:

            A8,9,10,11,13 low, A12 high

            (FIXME: why is the VIC only access on address 9000 (A15=1, A14=0),
            and not on 5000 (A15=0, A14=1) and D000 (A15=1, A14=1)???)

            NOTE: Unlike a real VIC, the VIC emulation has a traditional
            chip-select pin.
        */
        if (M6561_SELECTED_ADDR(pins)) {
            vic_pins |= M6561_CS;
        }
        if (pins & M6502_A4) {
            via1_pins |= M6522_CS1;
        }
        if (pins & M6502_A5) {
            via2_pins |= M6522_CS1;
        }
    }
    else {
        // regular memory access
        const uint16_t addr = M6502_GET_ADDR(pins);
        if (pins & M6502_RW) {
            M6502_SET_DATA(pins, mem_rd(&sys->mem_cpu, addr));
        }
        else {
            mem_wr(&sys->mem_cpu, addr, M6502_GET_DATA(pins));
        }
    }

    /* tick VIA1

        VIA1 IRQ pin is connected to CPU NMI pin

        VIA1 Port A input:
            PA0:    Serial CLK (FIXME)
            PA1:    Serial DATA (FIXME)
            PA2:    in: JOY0 (up)
            PA3:    in: JOY1 (down)
            PA4:    in: JOY2 (left)
            PA5:    in: PEN/BTN (fire)
            PA6:    in: CASS SENSE
            PA7:    SERIAL ATN OUT (???)

            CA1:    in: RESTORE KEY(???)
            CA2:    out: CASS MOTOR

        VIA1 Port B input:
            all connected to USR port

        NOTE: the IRQ/NMI mapping is reversed from the C64
    */
    {
        // FIXME: SERIAL PORT
        // FIXME: RESTORE key to M6522_CA1
        via1_pins |= (M6522_PA0 | M6522_PA1 | M6522_PA7);
        via1_pins = m6522_tick(&sys->via_1, via1_pins);
        if (via1_pins & M6522_IRQ) {
            pins |= M6502_NMI;
        }
        if ((via1_pins & (M6522_CS1 | M6522_RW)) == (M6522_CS1 | M6522_RW)) {
            pins = M6502_COPY_DATA(pins, via1_pins);
        }
    }

    /* tick VIA2

        VIA2 IRQ pin is connected to CPU IRQ pin

        VIA2 Port A input:
            read keyboard matrix rows

            CA1: in: CASS READ

        VIA2 Port B input:
            PB7:    JOY3 (Right)

        VIA2 Port A output:
            ---(???)

        VIA2 Port B output:
            write keyboard matrix columns
            PB3 -> CASS WRITE (not implemented)
    */
    {
        uint8_t kbd_lines = ~kbd_scan_lines(&sys->kbd);
        M6522_SET_PA(via2_pins, kbd_lines);
        via2_pins = m6522_tick(&sys->via_2, via2_pins);
        uint8_t kbd_cols = ~M6522_GET_PB(via2_pins);
        kbd_set_active_columns(&sys->kbd, kbd_cols);
        if (via2_pins & M6522_IRQ) {
            pins |= M6502_IRQ;
        }
        if ((via2_pins & (M6522_CS1 | M6522_RW)) == (M6522_CS1 | M6522_RW)) {
            pins = M6502_COPY_DATA(pins, via2_pins);
        }
    }

    // tick the VIC
    {
        vic_pins = m6561_tick(&sys->vic, vic_pins);
        if ((vic_pins & (M6561_CS | M6561_RW)) == (M6561_CS | M6561_RW)) {
            pins = M6502_COPY_DATA(pins, vic_pins);
        }
        if (vic_pins & M6561_SAMPLE) {
            sys->audio.sample_buffer[sys->audio.sample_pos++] = sys->vic.sound.sample;
            if (sys->audio.sample_pos == sys->audio.num_samples) {
                if (sys->audio.callback.func) {
                    sys->audio.callback.func(
                        sys->audio.sample_buffer,
                        sys->audio.num_samples,
                        sys->audio.callback.user_data);
                }
                sys->audio.sample_pos = 0;
            }
        }
    }

    return pins;
}

uint32_t x65_exec(x65_t* sys, uint32_t micro_seconds) {
    CHIPS_ASSERT(sys && sys->valid);
    uint32_t num_ticks = clk_us_to_ticks(X65_FREQUENCY, micro_seconds);
    uint64_t pins = sys->pins;
    if (0 == sys->debug.callback.func) {
        // run without debug callback
        for (uint32_t ticks = 0; ticks < num_ticks; ticks++) {
            pins = _x65_tick(sys, pins);
        }
    }
    else {
        // run with debug callback
        for (uint32_t ticks = 0; (ticks < num_ticks) && !(*sys->debug.stopped); ticks++) {
            pins = _x65_tick(sys, pins);
            sys->debug.callback.func(sys->debug.callback.user_data, pins);
        }
    }
    sys->pins = pins;
    kbd_update(&sys->kbd, micro_seconds);
    return num_ticks;
}

static uint16_t _x65_vic_fetch(uint16_t addr, void* user_data) {
    x65_t* sys = (x65_t*)user_data;
    uint16_t data = (sys->color_ram[addr & 0x03FF] << 8) | mem_rd(&sys->mem_vic, addr);
    return data;
}

static void _x65_init_key_map(x65_t* sys) {
    kbd_init(&sys->kbd, 1);
    const char* keymap =
        // no shift
        //   01234567 (col)
        "1     Q2"  // row 0
        "3WA ZSE4"  // row 1
        "5RDXCFT6"  // row 2
        "7YGVBHU8"  // row 3
        "9IJNMKO0"  // row 4
        "+PL,.:@-"  // row 5
        "~*;/ =  "  // row 6, ~ is british pound
        "        "  // row 7

        /* shift */
        "!     q\""
        "#wa zse$"
        "%rdxcft^"
        "&ygvbhu*"
        "(ijnmko)"
        " pl<>[  "
        "$ ]?    "
        "        ";
    CHIPS_ASSERT(strlen(keymap) == 128);
    // shift is column 3, line 1
    kbd_register_modifier(&sys->kbd, 0, 3, 1);
    // ctrl is column 2, line 0
    kbd_register_modifier(&sys->kbd, 1, 2, 0);
    for (int shift = 0; shift < 2; shift++) {
        for (int column = 0; column < 8; column++) {
            for (int line = 0; line < 8; line++) {
                int c = keymap[shift * 64 + line * 8 + column];
                if (c != 0x20) {
                    kbd_register_key(&sys->kbd, c, column, line, shift ? (1 << 0) : 0);
                }
            }
        }
    }

    // special keys
    kbd_register_key(&sys->kbd, 0x20, 4, 0, 0);  // space
    kbd_register_key(&sys->kbd, 0x08, 2, 7, 1);  // cursor left
    kbd_register_key(&sys->kbd, 0x09, 2, 7, 0);  // cursor right
    kbd_register_key(&sys->kbd, 0x0A, 3, 7, 0);  // cursor down
    kbd_register_key(&sys->kbd, 0x0B, 3, 7, 1);  // cursor up
    kbd_register_key(&sys->kbd, 0x01, 0, 7, 0);  // delete
    kbd_register_key(&sys->kbd, 0x0D, 1, 7, 0);  // return
    kbd_register_key(&sys->kbd, 0x03, 3, 0, 0);  // stop
    kbd_register_key(&sys->kbd, 0xF1, 4, 7, 0);
    kbd_register_key(&sys->kbd, 0xF2, 4, 7, 1);
    kbd_register_key(&sys->kbd, 0xF3, 5, 7, 0);
    kbd_register_key(&sys->kbd, 0xF4, 5, 7, 1);
    kbd_register_key(&sys->kbd, 0xF5, 6, 7, 0);
    kbd_register_key(&sys->kbd, 0xF6, 6, 7, 1);
    kbd_register_key(&sys->kbd, 0xF7, 7, 7, 0);
    kbd_register_key(&sys->kbd, 0xF8, 7, 7, 1);
}

bool x65_quickload(x65_t* sys, chips_range_t data) {
    CHIPS_ASSERT(sys && sys->valid && data.ptr && (data.size > 0));
    if (data.size < 2) {
        return false;
    }
    const uint8_t* ptr = (uint8_t*)data.ptr;
    const uint16_t start_addr = ptr[1] << 8 | ptr[0];
    ptr += 2;
    const uint16_t end_addr = start_addr + (data.size - 2);
    uint16_t addr = start_addr;
    while (addr < end_addr) {
        mem_wr(&sys->mem_cpu, addr++, *ptr++);
    }
    return true;
}

void x65_key_down(x65_t* sys, int key_code) {
    CHIPS_ASSERT(sys && sys->valid);
    kbd_key_down(&sys->kbd, key_code);
}

void x65_key_up(x65_t* sys, int key_code) {
    CHIPS_ASSERT(sys && sys->valid);
    kbd_key_up(&sys->kbd, key_code);
}

chips_display_info_t x65_display_info(x65_t* sys) {
    chips_display_info_t res = {
        .frame = {
            .dim = {
                .width = M6561_FRAMEBUFFER_WIDTH,
                .height = M6561_FRAMEBUFFER_HEIGHT,
            },
            .bytes_per_pixel = 1,
            .buffer = {
                .ptr = sys ? sys->fb : 0,
                .size = M6561_FRAMEBUFFER_SIZE_BYTES,
            }
        },
        .palette = m6561_palette(),
    };
    if (sys) {
        res.screen = m6561_screen(&sys->vic);
    }
    else {
        res.screen = (chips_rect_t){
            .x = 0,
            .y = 0,
            .width = _X65_SCREEN_WIDTH,
            .height = _X65_SCREEN_HEIGHT,
        };
    }
    CHIPS_ASSERT(((sys == 0) && (res.frame.buffer.ptr == 0)) || ((sys != 0) && (res.frame.buffer.ptr != 0)));
    return res;
}

uint32_t x65_save_snapshot(x65_t* sys, x65_t* dst) {
    CHIPS_ASSERT(sys && dst);
    *dst = *sys;
    chips_debug_snapshot_onsave(&dst->debug);
    chips_audio_callback_snapshot_onsave(&dst->audio.callback);
    m6502_snapshot_onsave(&dst->cpu);
    m6561_snapshot_onsave(&dst->vic);
    mem_snapshot_onsave(&dst->mem_cpu, sys);
    mem_snapshot_onsave(&dst->mem_vic, sys);
    return X65_SNAPSHOT_VERSION;
}

bool x65_load_snapshot(x65_t* sys, uint32_t version, x65_t* src) {
    CHIPS_ASSERT(sys && src);
    if (version != X65_SNAPSHOT_VERSION) {
        return false;
    }
    static x65_t im;
    im = *src;
    chips_debug_snapshot_onload(&im.debug, &sys->debug);
    chips_audio_callback_snapshot_onload(&im.audio.callback, &sys->audio.callback);
    m6502_snapshot_onload(&im.cpu, &sys->cpu);
    m6561_snapshot_onload(&im.vic, &sys->vic);
    mem_snapshot_onload(&im.mem_cpu, sys);
    mem_snapshot_onload(&im.mem_vic, sys);
    *sys = im;
    return true;
}

#endif  // CHIPS_IMPL
