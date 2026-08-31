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

| Command    | Purpose                                                                                                |
| ---------- | ------------------------------------------------------------------------------------------------------ |
| `load`     | Load a `.xex` into PSRAM and jump to its entry point (bare-metal; single session)                      |
| `info`     | Show the INFO-segment metadata of a `.xex` without running it (see [Appendix F](../A/F_xex_format.md)) |
| `install`  | Install a `.xex` into the LittleFS boot catalogue                                                      |
| `remove`   | Remove a `.xex` from the boot catalogue                                                                |

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

## DE-9 GPIO Ports

The board carries two **DE-9 connectors** in the classic double-joystick-port arrangement — but read the label carefully: these are **general-purpose, bi-directional GPIO ports** first and joystick ports second. Each of the sixteen lines can be individually configured as an input or an output, and the port side runs at **5 V TTL levels**. In the spirit of the C64's control and user ports, the same connectors that accept an Atari-style stick can just as well read a sensor, switch a relay driver, or bit-bang a home-built peripheral — the retro joystick pinout is simply the default *convention* for how the pins are used, not what the hardware is.

Behind the connectors sits an NXP **PCAL6416A**, a 16-bit I²C GPIO expander with built-in voltage level translation: its I²C side talks to the NORTH chip at 3.3 V while its port side runs from the 5 V rail. Expander port 0 is the first DE-9 connector and port 1 the second, eight lines each. The expander's other defining property for the X65 is its **interrupt-mask registers** — software requests IRQs only for the pin transitions it cares about, avoiding the paging-on-every-pin-change behaviour of simpler expanders.

### Connector pinout

Port bits map to DE-9 pins in order; pin 8 is ground, so the eight GPIO lines occupy pins 1–7 and 9:

| DE-9 pin | Port bit | Joystick convention        | As GPIO             |
| -------- | -------- | -------------------------- | ------------------- |
| 1        | 0        | Up                         | GPIO line 0         |
| 2        | 1        | Down                       | GPIO line 1         |
| 3        | 2        | Left                       | GPIO line 2         |
| 4        | 3        | Right                      | GPIO line 3         |
| 5        | 4        | Button 3                   | GPIO line 4         |
| 6        | 5        | Fire (button 1)            | GPIO line 5         |
| 7        | 6        | Button 4                   | GPIO line 6         |
| 8        | —        | Ground                     | Ground              |
| 9        | 7        | Button 2                   | GPIO line 7         |

Joystick inputs are **active-low**: a closed switch shorts the pin to ground (pin 8), so with the pull-ups enabled a pressed direction or button reads `0`. Note that on classic machines pin 7 carried +5 V; on the X65 it is a full GPIO line, used as button 4 under the joystick convention. Pins 5 and 9 — the analog paddle inputs of the old ports — are plain digital lines here, used as buttons 3 and 2.

Because the port side runs at 5 V, classic-machine accessories that expect power on pin 7 are still an option: configure bit 6 as an output and drive it permanently high, and the pin becomes the +5 V supply the old ports provided. That is enough for the light electronics of the era — autofire circuits, adapter dongles — within the expander's drive limits (see below). The trade-off is losing button 4 on that connector, which classic devices never had anyway.

### Register file

The expander's complete register file is memory-mapped at **`$FF80–$FF97`**; the 65C816 sees plain bank-0 addresses while the firmware relays accesses over I²C. Registers come in pairs — one per port / connector, suffix `0` and `1`:

| Address           | Register            | Reset | Function                                                                                              |
| ----------------- | ------------------- | ----- | ----------------------------------------------------------------------------------------------------- |
| `$FF80` / `$FF81` | `IN0` / `IN1`       | live  | **Input port** (read-only). The actual pin levels, regardless of direction. Reading clears a pending interrupt for that port. |
| `$FF82` / `$FF83` | `OUT0` / `OUT1`     | `$FF` | **Output port.** Level driven on pins configured as outputs; bits for input pins are ignored. Reads return what was written, not the pin. |
| `$FF84` / `$FF85` | `POL0` / `POL1`     | `$00` | **Polarity inversion.** `1` inverts that input pin's sense as seen in `IN` — handy for reading active-low joystick switches as `1` = pressed. |
| `$FF86` / `$FF87` | `CFG0` / `CFG1`     | `$FF` | **Direction.** `1` = high-impedance input (the reset state), `0` = output.                            |
| `$FF88`–`$FF8B`   | `STR0L/H`, `STR1L/H`| `$FF` | **Output drive strength**, two bits per pin (`STRxL` covers bits 0–3, `STRxH` bits 4–7): `00` = ¼, `01` = ½, `10` = ¾, `11` = full drive. |
| `$FF8C` / `$FF8D` | `LTCH0` / `LTCH1`   | `$00` | **Input latch.** `1` = a level change is captured into `IN` and held until `IN` is read, so a short pulse cannot slip between polls. `0` = `IN` tracks the pin live. |
| `$FF8E` / `$FF8F` | `PLE0` / `PLE1`     | `$00` | **Pull resistor enable.** `1` connects the pin's internal 100 kΩ resistor.                            |
| `$FF90` / `$FF91` | `PLS0` / `PLS1`     | `$FF` | **Pull direction.** `1` = pull-up, `0` = pull-down (effective only where `PLE` is set).               |
| `$FF92` / `$FF93` | `INTE0` / `INTE1`   | `$FF` | **Interrupt mask.** `0` = a change on that input asserts the IRQ. All pins power up masked — no spurious interrupts at boot. |
| `$FF94` / `$FF95` | `INST0` / `INST1`   | `$00` | **Interrupt status** (read-only). `1` = this pin is the source of the pending interrupt.              |
| `$FF96`           | —                   | —     | Reserved.                                                                                             |
| `$FF97`           | `OUTCF`             | `$00` | **Output stage configuration**, per port: bit 0 = port 0, bit 1 = port 1; `0` = push-pull, `1` = open-drain (set it *before* switching pins to output). |

The reset state is deliberately benign: every pin an input, pulls disconnected, every interrupt masked. Reading a joystick therefore takes two setup writes — enable the pull-ups (`PLS` already defaults to "up", so just set `PLE` to `$FF`) — after which `IN0`/`IN1` deliver the stick state directly. Driving external hardware is the mirror image: write the level to `OUT`, then clear the pin's `CFG` bit to turn the driver on.

### Electrical limits

For the "control external devices" use case, the numbers that matter (from the PCAL6416A datasheet):

- Inputs are high-impedance and tolerate up to 5.5 V regardless of the port supply.
- Internal pull resistors are nominally 100 kΩ (50–150 kΩ across conditions).
- Outputs sink up to **25 mA per pin** — enough to drive an LED directly — but keep each 8-pin port under 100 mA total, and mind the per-pin drive-strength setting.
- Open-drain mode (`OUTCF`) disconnects the internal pulls on that port; provide an external pull-up when wire-OR-ing.

### Interrupts

The expander's `INT` line feeds **bit 1 of the RIA interrupt controller** (`$FFEC` mask / `$FFED` status — see the [UART section](#uart-and-the-monitor-console) above). Unmasking a pin in `INTE` makes any change on it assert the IRQ; the handler reads `INST0`/`INST1` to find which pin fired and then reads `IN` to clear the condition. Combined with the input latch this gives fully edge-driven joystick handling — no per-frame polling required.

:::{note}
The NORTH firmware does not decode this window yet — reads currently return `$FF` and writes are ignored. Until the I²C path to the expander is brought up, use USB HID gamepads via `$FFB0–$FFBF` for controller input.
:::

## RGB LED Chain

The DEV-board has three on-board RGB LEDs (intended for keyboard state on the full machine) plus a WS2812 data line on the expansion port, which together support a chain of up to 256 addressable LEDs. The RIA decodes and the firmware drives **four** direct LED registers; the fourth position is simply not populated on the DEV-board, so `$FFA3` addresses an LED you can solder onto the strip header yourself. The CPU reaches them through two parallel interfaces at **`$FFA0–$FFA7`**:

### Direct RGB332 for LEDs 0–3

| Address | LED | Byte format            |
| ------- | --- | ---------------------- |
| `$FFA0` | 0   | `[7:5] R · [4:2] G · [1:0] B` |
| `$FFA1` | 1   | same                   |
| `$FFA2` | 2   | same                   |
| `$FFA3` | 3   | same                   |

A single `STA` lights up one of the first four LEDs — no sequencing required. LED 3 is unpopulated on the DEV-board; the write still reaches the chain.

### Chain protocol (any LED 0–255, full 24-bit colour)

| Address | Role                                                                   |
| ------- | ---------------------------------------------------------------------- |
| `$FFA4` | LED index in the chain — **writing this commits the update**           |
| `$FFA5` | Red byte (latched)                                                     |
| `$FFA6` | Green byte (latched)                                                   |
| `$FFA7` | Blue byte (latched)                                                    |

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

Each of the four I/O slots on the connector gets its own enable and interrupt line, meaning up to four cards can be installed at once with independent address decoding and IRQ routing. CPU-bus expansion boards see the same 8-bit data and low-address signalling as the on-board chips, so a custom peripheral can map itself into the `$FC00–$FDFF` expansion window using its own `IO_EN` line — one 128-byte slot per card. See [Appendix A](../A/A_memory_map.md) for the slot ranges and the RIA chunk bitmap that decides which parts of the window reach the bus.

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
