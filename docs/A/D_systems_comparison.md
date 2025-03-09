# Appendix D: X65 Compared to Classic 8-bit Systems

## Introduction

This chapter provides detailed comparisons between the X65 microcomputer and several classic 8-bit systems, highlighting similarities, differences, and advantages. These comparisons can help developers familiar with vintage platforms understand how their knowledge transfers to the X65 environment.

## CPU Comparison

| Feature              | X65 (65C816)              | Commodore 64 (6510) | Apple IIe (65C02)   | Atari 800XL (6502) | SNES (5A22/65C816)   |
| -------------------- | ------------------------- | ------------------- | ------------------- | ------------------ | -------------------- |
| Word Size            | 8/16-bit                  | 8-bit               | 8-bit               | 8-bit              | 8/16-bit             |
| Address Space        | 16MB                      | 64KB                | 64KB                | 64KB               | 24-bit address bus   |
| Clock Speed          | ~7MHz                     | ~1MHz               | 1MHz                | 1.79MHz            | 3.58MHz              |
| Direct Page          | Relocatable               | Fixed Zero Page     | Fixed Zero Page     | Fixed Zero Page    | Relocatable          |
| Index Registers      | 8/16-bit X, Y             | 8-bit X, Y          | 8-bit X, Y          | 8-bit X, Y         | 8/16-bit X, Y        |
| CPU Modes            | Native Mode               | N/A                 | N/A                 | N/A                | Emulation/Native     |
| Stack Size           | 64KB                      | 256 bytes           | 256 bytes           | 256 bytes          | 64KB                 |
| Special Instructions | WAI, Block move (MVN/MVP) | None                | WAI, STP, BBR, etc. | None               | Block move (MVN/MVP) |

## Memory Architecture

| Feature        | X65                       | Commodore 64       | Apple IIe         | Atari 800XL       | SNES                      |
| -------------- | ------------------------- | ------------------ | ----------------- | ----------------- | ------------------------- |
| Total RAM      | 16MB                      | 64KB               | 64KB + 128KB aux  | 64KB              | 128KB                     |
| ROM            | None (Loaded from SD)     | 20KB               | 16KB              | 24KB              | Game cartridge            |
| Memory Model   | Flat, Linear              | Banked             | Paged             | Cartridge + RAM   | Banked                    |
| I/O Method     | Memory-mapped             | Memory-mapped      | Memory-mapped     | Memory-mapped     | Memory-mapped             |
| Banking System | 256 banks of 64KB         | ROM/RAM/IO banking | Soft switches     | ANTIC/GTIA/PIA    | Various controllers       |
| Special Areas  | Direct Page (relocatable) | Zero Page (fixed)  | Zero Page (fixed) | Zero Page (fixed) | Direct Page (relocatable) |

## Graphics Capabilities

| Feature          | X65 (CGIA)                                     | C64 (VIC-II)           | Apple IIe                             | Atari 800XL (ANTIC/GTIA)      | SNES (PPU)                    |
| ---------------- | ---------------------------------------------- | ---------------------- | ------------------------------------- | ----------------------------- | ----------------------------- |
| Resolution       | Up to 768×240 (×480 interlace)                 | 320×200, 160×200       | 280×192, 560×192                      | 320×192, 80×192 (ANTIC modes) | Up to 512×478                 |
| Colors           | 256 colors                                     | 16 colors              | 16 colors (Lo-Res), 6 colors (Hi-Res) | 256 colors (GTIA modes)       | 32,768 colors (256 on-screen) |
| Sprites          | 8 sprites (up to 64px width) with multiplexing | 8 sprites (24×21)      | Software only                         | 4 missiles, 4 players         | 128 sprites (32×32)           |
| Text Modes       | 48×30, 96×30 characters                        | 40×25 characters       | 40×24, 80×24 characters               | Multiple ANTIC modes          | 32×28 characters              |
| Special Effects  | HAM mode, MODE7, Raster/DL interrupts          | Raster interrupts, FLI | Double hi-res                         | Display list, GTIA modes      | Mode 7, scaling, rotation     |
| Rendering System | Display list                                   | Raster-based           | Memory-mapped                         | Display list                  | Tile-based + sprites          |
| Graphics Memory  | Any two banks of 16MB RAM available            | Fixed VIC layout       | Fixed memory pages                    | ANTIC memory map              | VRAM with DMA                 |

## Sound and Audio

| Feature         | X65                         | Commodore 64               | Apple IIe               | Atari 800XL      | SNES                     |
| --------------- | --------------------------- | -------------------------- | ----------------------- | ---------------- | ------------------------ |
| Primary Sound   | Yamaha SD-1 FM              | SID                        | Speaker                 | POKEY            | SPC700                   |
| Channels        | 16 FM channels + EQ         | 3 voices + filters         | 1 speaker               | 4 channels       | 8 channels               |
| Secondary Sound | 2 PWM channels              | -                          | Mockingboard (optional) | -                | -                        |
| Sound Synthesis | FM, 4-operator              | Subtractive, 3 oscillators | Square wave             | Filtered digital | ADPCM samples            |
| Volume Control  | Per-channel + master, Mixer | Per-channel + master       | Single level            | Per-channel      | Per-channel + master     |
| Effects         | Built-in EQ, vibrato        | Filters, ring mod          | None built-in           | Distortion       | Echo, reverb, FIR filter |
| Note Range      | Full MIDI range             | 8 octaves                  | Limited                 | 3.5 octaves      | Full MIDI range          |

## Input/Output Capabilities

| Feature      | X65               | Commodore 64             | Apple IIe        | Atari 800XL          | SNES               |
| ------------ | ----------------- | ------------------------ | ---------------- | -------------------- | ------------------ |
| Keyboard     | USB, Built-in     | Built-in                 | Built-in         | Built-in             | Controllers only   |
| Joystick     | 2 DE-9 ports      | 2 DE-9 ports             | 16-pin ports     | 2 DE-9 ports         | 2 controller ports |
| Storage      | USB Pen Drive     | Datasette, Disk Drive    | Disk Drive       | Cassette, Disk Drive | Game Pak           |
| Serial       | USB-UART          | Serial IEC               | Serial           | SIO                  | -                  |
| Networking   | Wi-Fi, Bluetooth  | Cartridges/adapters      | Optional cards   | -                    | -                  |
| Expansion    | CPU bus expansion | Cartridge/Expansion port | Expansion slots  | Cartridge port, ECI  | -                  |
| Video Output | DVI-D             | Composite/RF             | Composite        | Composite/RF         | Composite/RGB      |
| Audio Output | Stereo RCA        | Mono                     | Built-in speaker | Mono                 | Stereo             |

## Programming Environment

| Feature             | X65                         | Commodore 64             | Apple IIe              | Atari 800XL             | SNES                  |
| ------------------- | --------------------------- | ------------------------ | ---------------------- | ----------------------- | --------------------- |
| Native Language     | 65816 Assembly              | 6510 Assembly            | 6502 Assembly          | 6502 Assembly           | 65816 Assembly        |
| High-level          | NeoBASIC, C                 | BASIC                    | BASIC, Pascal          | BASIC                   | C                     |
| Development Tools   | Modern assemblers, emulator | BASIC, ML monitors       | BASIC, monitors        | BASIC, Assembler Editor | Custom dev kits       |
| Debugging           | Monitor, emulator tools     | Machine language monitor | Monitor program        | DEBUG cartridge         | Special hardware      |
| Cross-development   | Full modern toolchain       | Modern cross-dev tools   | Modern cross-dev tools | Modern cross-dev tools  | Emulators, assemblers |
| Community Resources | GitHub, Discord             | Massive legacy + active  | Active community       | Active community        | ROM hacking community |
