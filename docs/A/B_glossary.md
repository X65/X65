# Appendix B: Glossary of X65 Terms

## CPU and Memory Terms

**65C816** - The Western Design Center 16-bit microprocessor that serves as the CPU for the X65, offering backward compatibility with the 6502 while providing 16-bit capabilities and a 16MB address space.

**Accumulator** - The primary register used for arithmetic and logical operations in the 65C816 CPU.

**Bank** - A 64KB segment of memory in the 65C816's address space. The X65 supports 256 banks for a total of 16MB.

**Direct Page** - A 256-byte region of memory that can be located anywhere in bank 0, offering faster access for certain instructions (similar to the Zero Page in 6502 processors).

**Emulation Mode** - A compatibility mode of the 65C816 where it behaves like a 65C02 with an 8-bit accumulator and 16-bit address space.

**L2 Cache** - A 64 KB cache in the NORTH chip's internal SRAM, sitting between the 65C816 bus bridge and the external PSRAM. Its sole purpose is to amortise the QPI PSRAM protocol delays so that typical 65C816 access patterns do not pay full PSRAM latency. Transparent to application code.

**Native Mode** - The full 16-bit operating mode of the 65C816, which the X65 uses exclusively from boot-up.

**PSRAM** - Pseudo Static Random-Access Memory, the type of memory used in the X65 to provide 16MB of RAM with low power consumption. Organised as two 8 MB banks switched at address `$800000`.

**VAB (Valid Address Bus)** - A signal derived from the 65C816's VDA and VPA signals, used to optimize memory cycles when the CPU is performing internal operations.

**XSTACK** - A 512-byte auxiliary buffer maintained by the RIA firmware. Used by the OS fastcall API to pass call arguments and return data between 65816 software and the NORTH chip without touching the 6502 hardware stack.

## Graphics Terms

**80-column mode** - A readable 80-column text presentation on the 320-pixel-wide CGIA screen. Implemented by enabling **multi-color** on a text mode without double-width, which halves the pixel cell to 4×8 and yields exactly 80 columns. Typical fonts use only colour pairs 00 and 11 for a clean monochrome look.

**CGIA (Color Graphics Interface Adaptor)** - The X65's graphics subsystem, combining features from Atari ANTIC and Commodore VIC-II/TED architectures.

**Display List** - A structured set of instructions in memory that dictates how each scanline is rendered by the CGIA.

**Double-width text** - A per-plane flag (`PLANE_MASK_DOUBLE_WIDTH`) that doubles the horizontal pixel size of a text mode, producing the chunky "wide character" look familiar from Atari and C64 text modes. Available on every text mode.

**Affine Mode (MODE7)** - The CGIA's mode 7: a chunky-pixel graphics mode with hardware-assisted affine transformations, similar to the SNES MODE7.

**HAM6 (Hold-And-Modify, mode 6)** - A CGIA graphics mode inspired by the Amiga HAM mode. Uses 6-bit commands packing four screen pixels into three bytes; commands either select a base color or modify a single R/G/B channel of the previous pixel by a signed delta, enabling many more on-screen colors at the cost of per-pixel precision.

**Half-bright flag** - In CGIA 4 bpp paletted modes (MODE0 and MODE1), the highest bit of each cell's 4-bit colour code is not part of palette selection. It acts after the `shared_colors[]` lookup, XORing bit 2 of the resulting 8-bit CGIA palette index — a brightness bit in the 32 hue × 8 brightness layout — so the cell's colours flip to their bright/dark twin in the same hue row. Effectively, a 4 bpp cell picks one of eight `shared_colors[]` entries plus a half-bright transform, giving a 16-colour feel regardless of how `shared_colors[]` was loaded.

**LMS (Load Memory Scan)** - A display list instruction that defines the offset of display memory, holding character or tile data.

**MODE0** - The CGIA's paletted text/tile mode (mode slot 0). Uses eight palette entries stored in the upper half of the plane's 16 registers; selectable 1 / 2 / 3 / 4 bpp; character-generator bits supply the low bit(s) of the palette index while "stolen" high bits of the character code supply the high bits — so higher bpp means fewer usable glyphs, and consecutive char-code ranges paint with successive palette pairs (or quads, in the multi-color variant). Supports a **multi-color** variant restricted to 2 and 3 bpp. See [Chapter 4](../1/4_graphics.md#mode0-and-mode1-paletted-modes) for the per-bpp char-code layouts.

**MODE1** - The CGIA's paletted bitmap mode (mode slot 1). Same palette + bpp mechanics as MODE0, but bitmap bits directly index the palette — no character generator. 3 bpp uses HAM-style bitpacking (4 pixels per 3 bytes).

**MODE7** - Common name for the CGIA's affine mode (mode 7). See *Affine Mode*.

**Multicolor Mode** - A per-instruction flag bit on CGIA text and bitmap modes that switches to a 4-color-per-cell representation, where each byte encodes four pixels.

**Paletted mode** - A CGIA mode where each pixel selects one of a small palette table (8 entries) stored directly in the plane's registers, rather than looking up colour via a separate attribute memory area. See MODE0 and MODE1.

**Plane Order Register** - A single CGIA register at offset `$21` whose value selects one of 24 Z-order permutations of the four planes (using Steinhaus-Johnson-Trotter ordering). Lets a program shuffle which plane is on top — typically from a raster interrupt — without rebuilding any plane.

**Sprite Multiplexing** - A technique where sprite hardware is reused to display more sprites than hardware natively supports.

**VPU (Video Processing Unit)** - The hardware component that executes display list instructions and updates the screen accordingly.

## Audio Terms

**CODEC** - The analog-mixed-signal chip that converts the audio MCU's 48 kHz stereo I²S bitstream into analog line-out, and accepts analog line-in.

**FM Synthesis** - Frequency Modulation synthesis, the per-operator sound generation method at the heart of the X65's SGU-1 chip.

**Furnace Tracker** - An open-source multi-chip tracker composition tool. X65 maintains a port (`github.com/X65/furnace`) that adds SGU-1 as a supported chip, so songs can be written and exported directly against the real hardware's capabilities.

**I²S** - The three-wire digital audio link (bit clock, word clock, data) between the audio MCU and the CODEC. The X65 runs it at 48 kHz, 32-bit, master-mode from the audio MCU side.

**Paw key (🐾)** - The X65 keyboard's dedicated command-modifier key, equivalent in position to the Windows / Meta / ⌘ key on other platforms. Used for OS-level chord shortcuts — for example `🐾`+digit to switch virtual terminals.

**PWM (Pulse-Width Modulation)** - A technique for generating analog signals from digital devices. Used by the X65 system buzzer to produce simple tones.

**SGU-1 (Sound Generator Unit 1)** - The X65's custom synthesis chip. Nine stereo channels of four-operator FM with per-operator waveform selection (including PCM as a wavetable), per-operator ADSR envelopes, a SID-style multimode filter, three independent hardware sweeps per channel, and a shared 64 KB PCM bank. Implemented in firmware on a dedicated RP2350 audio chip that is attached to the SOUTH chip over an SPI link; reached from the CPU as the `SPU` device on the PIX bus, which SOUTH forwards over SPI to the audio chip.

**tSU (tildearrow Sound Unit)** - The fantasy synthesis chip from the Furnace tracker that the SGU-1 is conceptually based on. SGU-1 lifts the tSU operator model and wraps it in a memory-mapped register interface sized for X65 (9 channels, 4 operators).

## I/O and System Terms

**Clockport (on-board I²C header)** - A small on-board I²C header, inspired by the Amiga Clockport, for simple add-ons like real-time clocks and sensors. Shares the same I²C bus that runs over the expansion port.

**cc65-dbg** - A VS Code extension that adds a Debug Adapter Protocol (DAP) front end for cc65/ca65 workflows on the X65. Parses `ld65 --dbgfile` output and drives any DAP-capable emulator or adapter — including X65 Emu — to provide breakpoints, register and memory views, disassembly, watch expressions, and module-scoped globals.

**Raspberry Pi Radio Module 2** - The surface-mount Wi-Fi + Bluetooth module used by the X65. Based on CYW43-series silicon (the same wireless core as on the Raspberry Pi Pico W) and driven directly by the NORTH RP2350 over a PIO-bit-banged SPI link. The 65C816 reaches the networking stack through an AT-command interface carried on the main UART.

**DE-9** - The 9-pin connector used for joystick ports in the X65, compatible with Atari-style joysticks, supporting four-button input.

**DVI-D** - Digital Visual Interface, the video output standard used by the X65. Physically connected via an HDMI-shaped socket; only DVI-D TMDS is transmitted, so standard HDMI cables work without the X65 needing HDMI certification.

**Emu** - The X65 emulator. Compiles firmware C code directly for CGIA/SGU-1 bring-up parity, wires the chips together via the `chip-emulators` pin-mask model, and ships as native builds (Linux, Linux ARM, Windows) plus a WebAssembly build published at `x65.zone/emu`. Shares code paths with the real firmware wherever practical; HID support, for example, runs the same source with its event source swapped between libusb and the Web Gamepad API.

**EXPansion Port** - The X65's main expansion connector (physically a PCIe x4 slot repurposed for ease of sourcing). Exposes 8-bit data and address buses, CPU control lines (PHI2, /IRQB, /NMIB, /VAB, R/WB, /RESB, /ABORT, RDY, BE), four I/O-enable and four I/O-interrupt signals, +5 V / +3.3 V / GND, I²C, UART, stereo audio in/out, and a WS2812 LED data line.

**HID merged gamepad (pad 0)** - The virtual gamepad exposed at HID-selector value `$02` (device type 2, high-nibble pad index 0). Its report is the bitwise OR of all connected physical pads, so single-player code can read a single endpoint regardless of which controller is plugged in.

**K816** - A high-level assembler targeting the 65816, developed as part of the X65 project. Rust-based rewrite of K65 (originally an Atari VCS / 6502 assembler). Offers a C-like readable syntax, compile-time evaluation, structured blocks including far functions, a built-in formatter, and deterministic output suitable for golden testing.

**MEMTEST** - A monitor command that benchmarks and stress-tests both PSRAM banks, reporting copy speeds across SRAM, flash, and PSRAM (cached and uncached) plus a multi-block data-integrity test. Used to tune PSRAM timing parameters against the overclocked system clock.

**MMIO (Memory-Mapped I/O)** - A technique where hardware devices are accessed through memory addresses, used extensively in the X65.

**NORTH chip** - The RP2350 microcontroller in the X65 that owns the CPU bus, the PSRAM interface, and system services (RIA). Communicates with the SOUTH chip over the PIX bus.

**PCAL6416A** - The on-board 16-bit I²C GPIO expander, from NXP, that handles the DE-9 joystick inputs. Replaces an earlier TCA6416A. Its defining feature in X65 use is the built-in **interrupt-mask registers**: software requests IRQs only for the input transitions it cares about, rather than being paged for every pin change.

**PIX (Pico Information eXchange) bus** - The dedicated serial-with-handshake bus connecting the NORTH and SOUTH chips. Carries memory writes, DMA transfers, and per-device commands to the VPU (CGIA), SPU (SGU-1), and other south-side peripherals.

**RIA (Retro Interface Adaptor)** - The subsystem inside the NORTH chip that exposes the fastcall API at `$FFF0–$FFF3`, aggregates IRQ sources into a single CPU interrupt, and provides USB, storage, and other modern I/O capabilities to 65816 software.

**RP2350** - The Raspberry Pi microcontroller used in the X65 to implement the NORTH, SOUTH, and audio chips. Its Programmable I/O (PIO) blocks are how the X65 handles the CPU bus, the PIX bus, and several peripheral protocols in software.

**SOUTH chip** - The RP2350 microcontroller in the X65 that hosts the CGIA (graphics) implementation together with the terminal and font systems. Also bridges the separate SGU-1 audio chip via an SPI link, presenting it as the `SPU` device on the PIX bus. Reached from the CPU via the PIX bus.

**SPI (Serial Peripheral Interface)** - A synchronous serial communication interface used for connecting various components in the X65.

**UART (Universal Asynchronous Receiver/Transmitter)** - A serial communication protocol used for terminal access and debugging in the X65.

**USB-CDC serial** - The monitor console is reachable as a standard USB-CDC serial device over the same USB-C cable that powers the board. No separate debug header is needed: plug the X65 into a PC and the monitor port appears.

**WS2812B** - The serially-addressed RGB LED protocol used by the on-board LEDs and the RGB data line on the expansion port. Up to 256 LEDs in a chain are individually addressable through the chain-protocol registers at `$FFA4–$FFA7`.
