# Chapter 6: Input and Output Interfaces

The X65 surfaces its I/O through a small, disciplined register window at the top of bank 0 and a handful of physical connectors on the board. This chapter walks the hardware side of each subsystem — what it is, where it lives in memory, and how the pieces fit together. Assembly-level programming sequences for the same subsystems are covered in [Chapter 13: Input/Output Handling](../2/13_input_output.md).

## UART and the Monitor Console

The NORTH chip exposes a simple UART at **`$FFE0–$FFE1`** — two registers used to talk to the on-board monitor and, through it, to the OS and to applications.

| Address | Register         | R/W | Notes                                                              |
| ------- | ---------------- | --- | ------------------------------------------------------------------ |
| `$FFE0` | `UART_STATUS`    | R   | `[7]` TX writable (space in TX buffer), `[6]` RX ready (byte waiting) |
| `$FFE1` | `UART_RX`        | R   | Receive a byte; clears the RX-ready flag                           |
| `$FFE1` | `UART_TX`        | W   | Send a byte; dropped if `TX writable` was clear                    |

Polling is idiomatic: read `$FFE0`, test `[7]` before writing, test `[6]` before reading. A byte-per-call discipline keeps the firmware UART buffer happy.

The same UART register pair also carries the **USB-CDC serial** path. When the X65 is powered over USB-C, the connection doubles as a virtual serial port — plugging the DEV-board into a host PC brings up a CDC device that the monitor can be reached through directly, with no extra wiring.

A few neighbouring addresses share the `$FFE0` page:

| Address     | Function                                                   |
| ----------- | ---------------------------------------------------------- |
| `$FFE2–$FFE3` | Hardware random-number generator (read two bytes)        |
| `$FFEC`     | IRQ mask — enable/disable interrupt sources per bit        |
| `$FFED`     | IRQ status — which sources are currently asserting         |

## Monitor Commands

The monitor is reached through the same UART (`$FFE0–$FFE1`), whether attached physically or via the USB-CDC path. Two ways in:

- **From a host terminal over USB-CDC**: open the virtual serial port; the monitor prompt appears once a full system boot has happened.
- **From an attached screen + keyboard**: press **Ctrl-Alt-Delete**. The combination halts the 65C816 — the CPU is held in RESET — and switches the display / keyboard to the monitor console. Resetting or `reboot`-ing from the monitor releases the CPU back into normal execution.

The currently-implemented commands are:

### System

| Command     | Purpose                                        |
| ----------- | ---------------------------------------------- |
| `help`, `h`, `?` | List available commands                   |
| `status`    | Print system info: firmware build, CPU + core clocks, DVI mode, RAM size/topology |
| `set`       | Get or set a firmware configuration value      |
| `reboot`    | Restart the whole system                       |
| `reset`     | Reset only the 65C816                          |

### Filesystem (USB storage)

| Command            | Purpose                                             |
| ------------------ | --------------------------------------------------- |
| `ls`, `dir`        | List directory contents on the mounted USB volume   |
| `cd`, `chdir`      | Change the current working directory                |
| `mkdir`            | Create a directory                                  |
| `upload`           | Receive a file from the host over the serial link   |
| `unlink`           | Delete a file                                       |
| `binary`           | Switch the console into a binary transfer mode      |

### Boot ROM catalogue

These commands manage the `.xex` images that the NORTH firmware loads at power-on — the machine's built-in-ROM layer. On a stock X65, the shipped boot ROM is OS/816; normal user applications live *under* OS/816 and are not deployed here. See [Chapter 9](../2/09_dev_env.md) for the two deployment paths.

| Command    | Purpose                                                                    |
| ---------- | -------------------------------------------------------------------------- |
| `load`     | Load a `.xex` into PSRAM and jump to its entry point (bare-metal; single session) |
| `info`     | Show the INFO-segment metadata of a `.xex` without running it              |
| `install`  | Install a `.xex` into the LittleFS boot catalogue                          |
| `remove`   | Remove a `.xex` from the boot catalogue                                    |

### Diagnostics

| Command   | Purpose                                                                       |
| --------- | ----------------------------------------------------------------------------- |
| `memtest` | Benchmark and stress-test both PSRAM banks; report copy speeds and block-test results |

The combination of `upload` + `install` + `info` + `load` is the normal development loop: upload a freshly-built `.xex` from the host, inspect its metadata, load to run, install to persist.

## USB HID

USB keyboards, mice, and gamepads plugged into the X65 are handled by the NORTH chip's USB host stack and surfaced to the 65C816 as a 16-byte memory-mapped window at **`$FFB0–$FFBF`**.

The window is **device-switched**: writing to `$FFB0` picks which device appears in the other fifteen bytes. The selector byte packs two fields:

| Bits       | Meaning                                                                |
| ---------- | ---------------------------------------------------------------------- |
| `[3:0]`    | Device type — `0` keyboard, `1` mouse, `2` gamepad                     |
| `[7:4]`    | For keyboard: page index (0 or 1). For gamepad: pad number (0–4).      |

Keyboards expose a 32-byte state bitmap — one bit per HID keycode — split across two 16-byte pages. Page 0 also carries a small status byte at offset 0 (connected, NUMLOCK, CAPSLOCK, SCROLLLOCK).

Gamepads present a 10-byte report: D-pad and feature flags, digital stick directions, two button bytes, analog stick coordinates, and triggers. A writable selector value of `2` (high nibble zero) targets **pad 0 — the merged view**, whose bytes are the bitwise OR of every physically-connected gamepad. Single-player applications can read pad 0 unconditionally; multiplayer applications loop across pad indices 1–4.

Full register offsets and bit layouts live in [Appendix A: Memory Map](../A/A_memory_map.md); polling patterns are in [Chapter 13: Input/Output Handling](../2/13_input_output.md).

## DE-9 Joystick Ports

The board carries two Atari-style **DE-9 joystick ports**, four-button-capable, routed through an on-board I²C **PCAL6416A** GPIO expander. The expander's defining property for X65 is its **interrupt-mask registers** — software requests IRQs only for the pin transitions it cares about, avoiding the paging-on-every-pin-change behaviour of simpler expanders.

## RGB LED Chain

The DEV-board has three on-board RGB LEDs (intended for keyboard state on the full machine) plus a WS2812 data line on the expansion port, which together support a chain of up to 256 addressable LEDs. The CPU reaches them through two parallel interfaces at **`$FFA0–$FFA7`**:

### Direct RGB332 for LEDs 0–3

| Address | LED | Byte format            |
| ------- | --- | ---------------------- |
| `$FFA0` | 0   | `[7:5] R · [4:2] G · [1:0] B` |
| `$FFA1` | 1   | same                   |
| `$FFA2` | 2   | same                   |
| `$FFA3` | 3   | same                   |

A single `STA` lights up one of the first four LEDs — no sequencing required.

### Chain protocol (any LED 0–255, full 24-bit colour)

| Address | Role                                                                   |
| ------- | ---------------------------------------------------------------------- |
| `$FFA4` | Red byte — **writing this commits the update**                         |
| `$FFA5` | Green byte (latched)                                                   |
| `$FFA6` | Blue byte (latched)                                                    |
| `$FFA7` | LED index in the chain (latched)                                       |

Writing to `$FFA5`, `$FFA6`, or `$FFA7` only latches a value — no output is sent to the LED chain. Writing to `$FFA4` dispatches the PIX command to the SOUTH-side WS2812 driver with the current latched G / B / index and the freshly-written R.

Internally, both paths go out over the PIX bus as messages on the `MISC` device; the SOUTH chip drives the WS2812 line at the board's timing spec.

## System Buzzer

A small PWM-driven piezo **buzzer** on the board handles the kinds of tones an OS or application wants without needing to spin up an SGU-1 channel. Its registers live at **`$FFA8–$FFAB`**:

| Address | Register | Notes                                         |
| ------- | -------- | --------------------------------------------- |
| `$FFA8` | FREQ_L   | Low byte of the 16-bit frequency code         |
| `$FFA9` | FREQ_H   | High byte of the 16-bit frequency code        |
| `$FFAA` | DUTY     | Duty cycle, 0 (silent) – 255 (peak square wave) |
| `$FFAB` | —        | Reserved                                      |

The 16-bit `FREQ` value is mapped **logarithmically** to audio Hz, covering roughly 20 Hz to 20 kHz across the full range:

$$
f(\text{FREQ}) = 20\,\text{Hz} \cdot 2^{10\,\text{FREQ}/65535}
$$

Writing either `FREQ_L` or `FREQ_H` commits the new frequency via a PIX command to the SOUTH buzzer driver. Writing `DUTY` commits the new duty cycle independently — setting it to zero is the canonical way to silence a running tone without changing its pitch.

## Expansion Port

The X65's main expansion connector is physically a **PCIe x4 slot** (chosen for cheap, ubiquitous mechanical availability) repurposed as a CPU-bus expansion. The signal list exposes enough of the system bus for a wide range of add-on boards — memory, custom chips, external I/O:

| Group            | Signals                                                                |
| ---------------- | ---------------------------------------------------------------------- |
| Data bus         | `D0–D7`                                                                |
| Address bus      | `A0–A7`                                                                |
| CPU control      | `PHI2`, `/IRQB`, `/NMIB`, `/VAB`, `R/WB`, `/RESB`, `/ABORT`, `RDY`, `BE` |
| I/O enables      | `IO0_EN`, `IO1_EN`, `IO2_EN`, `IO3_EN` (four expansion slot selects)   |
| I/O interrupts   | `IO0_INT`, `IO1_INT`, `IO2_INT`, `IO3_INT` (one per slot)              |
| Power            | `+5 V`, `+3.3 V`, `GND`                                                |
| Other buses      | `I²C` (SDA / SCL), `UART` (TX / RX)                                   |
| Audio            | `MIX_OUT_L`, `MIX_OUT_R`, `EXT_IN_L`, `EXT_IN_R`, `AUDIO_EXT3`         |
| LEDs             | `WS2812` data line                                                     |

Each of the four I/O slots on the connector gets its own enable and interrupt line, meaning up to four cards can be installed at once with independent address decoding and IRQ routing. CPU-bus expansion boards see the same 8-bit data and low-address signalling as the on-board chips, so a custom peripheral can map itself into the `$FC00–$FCFF` expansion window using its own `IO_EN` line.

For a working reference: the board KiCad project and schematic PDF are published in the [X65 schematic repository](https://github.com/X65/schematic).

## On-Board I²C Header (Clockport)

Inspired by the Amiga Clockport, the DEV-board carries a small **on-board I²C header** intended for simple add-ons — real-time clocks, temperature sensors, EEPROMs — that do not warrant a full expansion-port slot. The header shares the same I²C bus that runs over the expansion port, so device addresses must be allocated to avoid collision across both surfaces.

Software reaches I²C peripherals via the RIA fastcall API rather than bit-banging the bus from the 65C816.

## Networking (Raspberry Pi Radio Module 2)

The board's wireless connectivity is provided by the **Raspberry Pi Radio Module 2** — a small ready-to-use surface-mount module carrying CYW43-series silicon, the same wireless core used by the Raspberry Pi Pico W. It is driven directly by the NORTH chip over a PIO-bit-banged SPI link (GPIOs 34–37).

Capabilities:

- **Wi-Fi**: TCP / UDP networking, IP configuration, DNS.
- **Bluetooth**: BLE and classic Bluetooth links.
- **AT command interface** — a text protocol for configuring the networking stack, parsed by NORTH. Modern extensions take the form `AT+XXX?` (query) and `AT+XXX=YYY` (set); traditional Hayes-like commands (`D`, `E`, `H`, `O`, `S`, `Z`, `&…`) remain available for modem-style dial-out paths.

Implemented AT+ extensions today include `AT+RF` and `AT+RFCC` (radio country code), `AT+SSID`, and `AT+PASS`.

For a program to reach the network layer, it issues AT commands via the UART (`$FFE0–$FFE1`) the same way it reaches the monitor; the firmware routes the bytes to the correct back-end based on whether the current console context is "monitor" or "modem".

## Summary

The X65 keeps its I/O surface shallow and direct: a dozen named MMIO windows at the top of bank 0, a monitor console reachable from the UART / USB-CDC path, a generous expansion connector exposing the raw 65C816 bus with I/O enables and interrupts, and a small I²C header for the kinds of add-ons a modern retro-style machine expects. [Chapter 13: Input/Output Handling](../2/13_input_output.md) shows how to actually talk to each of these from 65C816 assembly.
