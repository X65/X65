# Chapter 7: Development and Emulation

```{note}
This chapter is still under construction.
```

Developing for the X65 is a mix of classic 8-bit workflow — write 65C816 assembly or C, assemble, test — and modern conveniences: a proper emulator, a DAP-based debugger, a tracker for sound, and a choice of two assemblers depending on whether comfort or hand-rolled control matters more. This chapter tours the tools that make up that environment. Concrete setup steps and command lines are in [Chapter 9: Development Environment Setup](../2/09_dev_env.md); programming idioms for each subsystem are in Part 2.

## Emu — the X65 Emulator

**Emu** is the official X65 emulator and the primary way most developers see their code run. It is built on the `chip-emulators` toolbox: chips are modelled as small state machines and wired together through a **pin-mask model** — each cycle, every chip reads a shared bus state, decides what its pins should do, and writes them back, just as the real parts would on a breadboard. The result is fast, cycle-accurate enough for timing-sensitive code, and easy to extend when new chips land.

What makes Emu genuinely useful (rather than just another 65C816 simulator) is how much of the X65 it actually shares with the real hardware:

- **CGIA is the real firmware C code**, compiled into Emu rather than a separate emulator-only reimplementation. The handful of assembly-language inner loops in the firmware were rewritten to C so they fit both paths. This means a graphics bug seen on hardware reproduces in Emu, and a fix in either place lands in both.
- **SGU-1 runs the same DSP code** as the audio MCU. Emu renders its stereo 48 kHz output via the host's audio API.
- **USB HID uses the same source** as the firmware's HID layer, just with its event source swapped — `libusb` on native builds, the Web Gamepad API in the browser. Controller compatibility therefore matches the real board's, including merged-pad-0 semantics.

Emu is available for **Linux (x86_64)**, **Linux (ARM)**, and **Windows**.

From a workflow perspective: after the host's cc65 (or K816) toolchain emits a `.xex`, Emu is the fast iteration loop — edit, rebuild, drag-and-drop into Emu, observe. When the program behaves as intended in Emu, pushing it to real hardware via the monitor's `upload` / `install` / `load` path is a one-line difference. Because the chip cores are shared between the two targets, the hardware trip-up rate is low.

## Web Emulator

The same Emu also ships as a **WebAssembly build** hosted at **<https://x65.zone/emu/>**. The web version runs in any modern browser, loads `.xex` ROMs from URLs or drag-and-drop, and produces the same 768×480 picture and stereo audio as its desktop sibling. It is the primary way to share a working program without asking someone to install a toolchain.

A few things the web Emu is particularly good for:

- **Publishing example programs**: every post in the X65 blog that shows a running demo links to a playable web Emu session.
- **Bug reports**: "it reproduces here, _\[link\]_" is a much tighter feedback loop than explaining setup.
- **Teaching**: walk-throughs can run the code live in an embedded frame, no toolchain required.
- **First contact with the machine**: before buying a DEV-board, a prospective user can drive the real graphics / sound / HID behavior with a keyboard and a gamepad straight from their browser.

Because HID runs off the Web Gamepad API, the controller merge/select semantics on the web match the board: pad 0 returns the OR of any connected controllers; pads 1–4 return per-controller state.

## cc65-dbg — VS Code Debugger

**cc65-dbg** (<https://github.com/X65/cc65-dbg>) is a VS Code extension that adds a proper source-level debugger to the cc65/ca65 workflow. It speaks the **Debug Adapter Protocol (DAP)**, so it attaches to any DAP-compatible emulator or hardware adapter — in practice, Emu — and lets the editor drive step-through execution.

What it gives you:

- **Breakpoints** synchronised between the source and the running program.
- **Registers, memory, and disassembly** views updated live as you step.
- **Watch expressions** evaluated against the loaded debug info.
- **Module-scoped globals** — variables named and grouped by their defining source file, not just listed as a flat symbol dump.

cc65-dbg works by parsing the debug-info file emitted by `ld65 --dbgfile` alongside the `.xex`. The project then feeds that through DAP into whatever back-end the developer has attached. From the book's perspective, the important point is: the 6502/65C816 developer no longer needs to manually set breakpoints through a monitor-mode command loop — VS Code handles it the way it would for a modern program. See [Chapter 9](../2/09_dev_env.md) for the hookup steps.

## K816 — High-Level Assembler

**K816** (<https://github.com/X65/K816>) is a high-level assembler targeting the WDC 65C816. It is the direct descendant of **K65**, a long-running 6502 assembler for Atari VCS work, rewritten and extended so that 65C816 native-mode idioms — 24-bit addressing, 16-bit registers, relocatable direct page, structured functions — are first-class.

K816's design goals:

- **Readable, C-like source** for control flow and expression, without giving up the exact bit-for-bit control hand-written assembly provides.
- **Compile-time evaluation** for generating constants and data tables programmatically.
- **Structured blocks** — functions, inlining, and an explicit `far` function form for cross-bank calls.
- **Fast, small toolchain** written in modern Rust.

The significant thing about K816 for the X65 project is that **OS/816 is written entirely in it**. K816 is therefore battle tested in a complex project. Application and game programmers still have a choice: cc65/ca65 remains the main path for ordinary 65C816 programs, with K816 available for those who prefer its style.

A short taste of K816 source (a `Hello, UART` program) is walked through in [Chapter 9](../2/09_dev_env.md).

## Furnace Tracker Port

Music for the SGU-1 is composed in a **fork of Furnace Tracker** maintained at <https://github.com/X65/furnace>. Upstream Furnace is a modern, multi-chip tracker with support for classic Yamaha, SID, POKEY, and many other vintage sound chips. The X65 port adds SGU-1 as one of those chips, so songs can be written, auditioned, and exported against the real register behavior of the X65's sound hardware.

From a development workflow perspective the tracker replaces the "write a lot of register-poking code by hand" step: a composer writes a song in Furnace, exports it to a compact data format, and the 65C816-side player consumes that data and poll-drives the SGU-1. Assembly-level programming patterns for consuming tracker output are in [Chapter 12: Sound Programming](../2/12_sound.md).

## Monitor Console

The monitor is the **firmware-level** shell. Once a terminal is attached — either via USB-CDC on the DEV-board's USB-C connector or via the Ctrl-Alt-Delete key combination on a screen-plus-keyboard setup — its 16 commands provide everything needed to examine a running system and, importantly, to **manage the machine's boot-ROM catalogue**: the `.xex` images stored in internal LittleFS and loaded by NORTH firmware at boot. The default boot ROM is **OS/816** (see [Chapter 14](../2/14_os.md)); monitor commands like `install` / `remove` / `load` operate at that layer.

User-level applications — games, utilities, productivity tools that a normal X65 user would run — live **one layer up**, launched by OS/816's shell rather than by the firmware monitor. The build pipeline is the same (`.xex` from cc65 or K816); the deployment target and the invocation differ. Both paths are walked through concretely in [Chapter 9: Development Environment Setup](../2/09_dev_env.md).

The full monitor catalogue, grouped by purpose, is in [Chapter 6: Input and Output Interfaces](6_io.md); the assembly-side of reaching the monitor's UART from a program is in [Chapter 13](../2/13_input_output.md).

## Summary

The X65 development environment is built from a small number of pieces designed to feel modern without erasing the low-level character of the machine: **Emu** for fast iteration on desktop or in the browser, **cc65-dbg** for source-level debugging in VS Code, **cc65/ca65** as the default assembler, **K816** as the structured, Rust-based alternative that the OS itself is written in, **Furnace** for composing music, and the on-target **monitor** for moving artifacts between host and board. Setup and invocation details for each of these are covered in the Part 2 chapters that follow.
