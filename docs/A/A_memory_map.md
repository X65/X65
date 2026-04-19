# Appendix A: Memory Map

The X65 exposes a flat 24-bit address space to the 65C816. Most of that space is plain PSRAM; a 512-byte window at the top of bank 0 is carved up into memory-mapped-I/O regions owned by the custom chips, and a small expansion window sits slightly below it. This appendix documents every byte of that MMIO region.

The canonical live spreadsheet is at <https://tinyurl.com/x65-memory-map>:

<iframe width="100%" height="600" src="https://docs.google.com/spreadsheets/d/e/2PACX-1vRL2pdhGaOKfOaP7AGEpHNyDYzaYB4PVj7eJQSQAT52y95yiliqZ90WcLBRTPOZ93MihAFuKKmyBL1n/pubhtml?gid=0&amp;single=true&amp;widget=true&amp;headers=false"></iframe>

The tables below mirror that spreadsheet for offline and search-engine-friendly reference.

## Top-Level MMIO Layout

Bank 0, pages `$FC`–`$FF`:

| Range         | Owner                               | Notes                                                  |
| ------------- | ----------------------------------- | ------------------------------------------------------ |
| `$FC00–$FCFF` | Expansion slots                     | 16 slots × 16 bytes                                    |
| `$FEC0–$FEFF` | SGU-1 (sound)                       | 64-byte channel-switched window                        |
| `$FF00–$FF7F` | CGIA (graphics)                     | 128 registers                                          |
| `$FF80–$FF97` | GPIO expander / joystick            | Reserved (currently stubbed, reads return `$FF`)       |
| `$FF98–$FF9F` | System timers                       | Two CIA-compatible 16-bit counters, 1 µs resolution    |
| `$FFA0–$FFA7` | RGB LED chain                       | 4 direct RGB332 + 4-byte chain protocol                |
| `$FFA8–$FFAB` | System buzzer                       | 16-bit log frequency + 8-bit duty                      |
| `$FFAC–$FFAF` | Reserved                            | Reads return `$FF`                                     |
| `$FFB0–$FFBF` | USB HID (keyboard / mouse / gamepad)| Device-selector at `$FFB0`                             |
| `$FFC0–$FFFF` | RIA                                 | Fastcall API window at `$FFF0–$FFF3` within this range |

Everything outside the top half-page (`$FExx–$FFxx`) and the expansion slots at `$FCxx` is plain PSRAM and available to software. Bank crossing happens on the fly at `$800000`.

---

## `$FC00–$FCFF` — Expansion Slots

Sixteen 16-byte slots selected by address bits `$FC?0`–`$FC?F`. The expansion port routes four `IO_EN` signals and four `IO_INT` signals; which slot a given board responds to is board-specific. A typical OPL-3 card, for example, sits at `$FC00–$FC1F`.

No register layout is imposed by the core system — each expansion board defines its own map. See [Chapter 6: Input and Output Interfaces](../1/6_io.md) for the expansion port pinout.

---

## `$FEC0–$FEFF` — SGU-1

SGU-1 presents a **single 64-byte window** that is re-bound to a specific channel by writing a channel index to the last byte. Channels are numbered `0`–`8`; writing any value wraps modulo-9, so there is no special "service" channel index. The 65816 never sees a "global" SGU register file — everything is per-channel.

The selector:

| Offset         | Register      | R/W | Notes                                                          |
| -------------- | ------------- | --- | -------------------------------------------------------------- |
| `$3F` (`$FEFF`)| `CH_SELECT`   | R/W | Write: select channel (`0`–`8`, value wrapped mod 9). Read: returns currently selected channel. |

Once a channel is selected, the first 32 bytes (`$00–$1F`) are four **operators** of 8 bytes each; the next 32 bytes (`$20–$3F`) hold **channel-wide** controls.

### Operators (4 × 8 bytes at `$00–$1F`)

Each operator occupies eight bytes. Operator *n* starts at offset `0x08*n`.

| Offset | Register | Bit layout                                                |
| ------ | -------- | --------------------------------------------------------- |
| `+0`   | `R0`     | `[7] TRM` · `[6] VIB` · `[5:4] KSR` · `[3:0] MUL`         |
| `+1`   | `R1`     | `[7:6] KSL` · `[5:0] TL_lo`                               |
| `+2`   | `R2`     | `[7:4] AR_lo` · `[3:0] DR_lo`                             |
| `+3`   | `R3`     | `[7:4] SL` · `[3:0] RR`                                   |
| `+4`   | `R4`     | `[7:5] DT` · `[4:0] SR`                                   |
| `+5`   | `R5`     | `[7:5] DELAY` · `[4] FIX` · `[3:0] WPAR`                  |
| `+6`   | `R6`     | `[7] TRMD` · `[6] VIBD` · `[5] SYNC` · `[4] RING` · `[3:1] MOD` · `[0] TL_msb` |
| `+7`   | `R7`     | `[7:5] OUT` · `[4] AR_msb` · `[3] DR_msb` · `[2:0] WAVE`  |

The 5-bit envelope rates `AR` and `DR` are split across `R2` and `R7`. `TL` (Total Level) is 7 bits, split as `R1[5:0]` plus `R6[0]`. `SL` is 4 bits, `RR` 4 bits, `SR` 5 bits. `WAVE` selects one of eight per-operator waveforms (0 SINE, 1 TRIANGLE, 2 SAWTOOTH, 3 PULSE, 4 NOISE, 5 PERIODIC_NOISE, 6 reserved, 7 SAMPLE). `WPAR` shapes SINE/TRIANGLE/SAWTOOTH, picks a tap configuration for PERIODIC_NOISE, or sets a fixed pulse width for PULSE.

### Channel Controls (32 bytes at `$20–$3F`)

| Offset | Register         | Notes                                                                       |
| ------ | ---------------- | --------------------------------------------------------------------------- |
| `$20`  | `FREQ_L`         | Channel base frequency, low byte                                            |
| `$21`  | `FREQ_H`         | Channel base frequency, high byte                                           |
| `$22`  | `VOL`            | Channel volume (signed)                                                     |
| `$23`  | `PAN`            | Stereo pan (signed; negative = left, positive = right)                      |
| `$24`  | `FLAGS0`         | Gate, PCM-enable (bit 3), filter mode, ring modulation                      |
| `$25`  | `FLAGS1`         | Phase reset, filter reset, PCM loop, per-sweep enables                      |
| `$26`  | `CUTOFF_L`       | Filter cutoff, low byte                                                     |
| `$27`  | `CUTOFF_H`       | Filter cutoff, high byte                                                    |
| `$28`  | `DUTY`           | Pulse width for PULSE waveform (0–127)                                      |
| `$29`  | `RESON`          | Filter resonance (0–255; feedback is 256 − RESON)                           |
| `$2A`  | `PCM_POS_L`      | Current PCM sample position, low byte                                       |
| `$2B`  | `PCM_POS_H`      | Current PCM sample position, high byte                                      |
| `$2C`  | `PCM_END_L`      | PCM end boundary, low byte                                                  |
| `$2D`  | `PCM_END_H`      | PCM end boundary, high byte                                                 |
| `$2E`  | `PCM_RST_L`      | PCM loop restart, low byte; also the 1024-sample wavetable base for `SAMPLE` |
| `$2F`  | `PCM_RST_H`      | PCM loop restart, high byte                                                 |
| `$30`  | `SWFREQ_SPD_L`   | Frequency-sweep speed, low byte                                             |
| `$31`  | `SWFREQ_SPD_H`   | Frequency-sweep speed, high byte                                            |
| `$32`  | `SWFREQ_AMT`     | Frequency-sweep amount + direction/mode                                     |
| `$33`  | `SWFREQ_BND`     | Frequency-sweep boundary                                                    |
| `$34`  | `SWVOL_SPD_L`    | Volume-sweep speed, low byte                                                |
| `$35`  | `SWVOL_SPD_H`    | Volume-sweep speed, high byte                                               |
| `$36`  | `SWVOL_AMT`      | Volume-sweep amount + mode                                                  |
| `$37`  | `SWVOL_BND`      | Volume-sweep boundary                                                       |
| `$38`  | `SWCUT_SPD_L`    | Cutoff-sweep speed, low byte                                                |
| `$39`  | `SWCUT_SPD_H`    | Cutoff-sweep speed, high byte                                               |
| `$3A`  | `SWCUT_AMT`      | Cutoff-sweep amount + mode                                                  |
| `$3B`  | `SWCUT_BND`      | Cutoff-sweep boundary                                                       |
| `$3C`  | `RESTIMER_L`     | Phase-reset timer, low byte                                                 |
| `$3D`  | `RESTIMER_H`     | Phase-reset timer, high byte                                                |
| `$3E`  | *reserved*       |                                                                             |
| `$3F`  | `CH_SELECT`      | Channel selector (see above)                                                |

PCM sample data itself lives in 64 KB of RAM internal to the audio chip, addressed via the `PCM_POS` / `PCM_END` / `PCM_RST` pointers; it is **not** visible in the 65816's address space.

Writes to this window are intercepted by NORTH and forwarded to the audio chip via the `SPU` device on the PIX bus (see [Chapter 2: System Architecture Overview](../1/2_overview.md)). The audio chip is a dedicated RP2350 bridged from SOUTH over SPI; the SOUTH-side driver caches register values so repeat reads avoid a round-trip.

---

## `$FF00–$FF7F` — CGIA

CGIA exposes **128 registers**. The most relevant top-level ones:

| Offset | Register     | Notes                                                                                     |
| ------ | ------------ | ----------------------------------------------------------------------------------------- |
| `$00`  | `MODE`       | `[0] HIRES` (96-column / 768 px horizontal) · `[1] INTERLACE` (480-line vertical)         |
| `$21`  | `PLANE_ORDER`| Encodes one of 24 Z-order permutations of the four planes (Steinhaus-Johnson-Trotter)     |

The remaining registers fall into four per-plane banks of sixteen registers each; see [Chapter 4: Graphics and Display](../1/4_graphics.md) and [Chapter 11: Graphics Programming](../2/11_graphics.md) for the plane-register map, display-list instruction encoding, and sprite-descriptor format.

---

## `$FF80–$FF97` — GPIO Expander (reserved)

This 24-byte window is reserved for the on-board PCAL6416A GPIO expander that routes the DE-9 joystick inputs. In the current firmware the region is stubbed: all reads return `$FF` and writes are ignored. Until it is activated, use USB HID gamepads via `$FFB0–$FFBF`.

---

## `$FF98–$FF9F` — System Timers (CIA-Compatible)

Two 16-bit countdown timers with 1 µs resolution, modelled on the MOS 6526 CIA.

| Offset           | Register | R/W | Notes                                                                  |
| ---------------- | -------- | --- | ---------------------------------------------------------------------- |
| `$FF98`          | `TAL`    | R/W | Timer A counter, low byte                                              |
| `$FF99`          | `TAH`    | R/W | Timer A counter, high byte                                             |
| `$FF9A`          | `TBL`    | R/W | Timer B counter, low byte                                              |
| `$FF9B`          | `TBH`    | R/W | Timer B counter, high byte                                             |
| `$FF9C`          | —        | —   | Reserved, reads `$FF`                                                  |
| `$FF9D`          | `ICR`    | R/W | Interrupt control / flags                                              |
| `$FF9E`          | `CRA`    | R/W | Timer A control                                                        |
| `$FF9F`          | `CRB`    | R/W | Timer B control                                                        |

**Counter semantics.** Reading a counter returns the current remaining count (in µs). Writing the low byte latches it; writing the high byte loads the latched 16-bit pair into the counter when the timer is stopped, or sets the reload value used on underflow when running.

**`ICR` bits.** `[0]` Timer A underflow, `[1]` Timer B underflow, `[7]` any-interrupt summary. Reading `ICR` clears all pending flags. To set or clear interrupt enables: write with `[7]=1` to set the bits listed in `[1:0]`, `[7]=0` to clear them.

**Control registers (`CRA`/`CRB`).** `[0] START`, `[3] RUN_MODE` (0 continuous, 1 one-shot), `[4] FORCE_LOAD`. `CRB` additionally has `[6:5] INPUT_MODE` (0 counts PHI2, 2 counts Timer A underflows, useful for compounding to a 32-bit period).

---

## `$FFA0–$FFA7` — RGB LED Chain

The X65 DEV-board carries four on-board WS2812B-style RGB LEDs and supports a chain of up to 256 LEDs via the expansion port's `WS2812` data line.

| Offset   | Register      | R/W | Notes                                                         |
| -------- | ------------- | --- | ------------------------------------------------------------- |
| `$FFA0`  | `LED0`        | R/W | Direct RGB332 colour for LED 0; write commits immediately     |
| `$FFA1`  | `LED1`        | R/W | Direct RGB332 colour for LED 1                                |
| `$FFA2`  | `LED2`        | R/W | Direct RGB332 colour for LED 2                                |
| `$FFA3`  | `LED3`        | R/W | Direct RGB332 colour for LED 3                                |
| `$FFA4`  | `RGB_R`       | R/W | Red byte (0–255); **writing here commits the chain update**   |
| `$FFA5`  | `RGB_G`       | R/W | Green byte (0–255); latched                                   |
| `$FFA6`  | `RGB_B`       | R/W | Blue byte (0–255); latched                                    |
| `$FFA7`  | `RGB_IDX`     | R/W | LED index in the chain (0–255); latched                       |

**RGB332 byte** (`$FFA0–$FFA3`): `[7:5] R` · `[4:2] G` · `[1:0] B`. A single `STA $FFA0` sets LED 0 — the LED 0–3 interface is one instruction per LED.

**Chain protocol** (`$FFA4–$FFA7`): to set LED *n* to 24-bit colour, write the LED index to `$FFA7`, the green byte to `$FFA5`, the blue byte to `$FFA6`, and finally the red byte to `$FFA4`. The final write to `$FFA4` is what dispatches the update to the hardware; the other three bytes are simply latched. (Order of the latch writes is free; only the write to `$FFA4` must be last.)

---

## `$FFA8–$FFAB` — System Buzzer

A PWM-driven piezo buzzer. Two commands are exposed to the CPU; each write forwards a PIX command to the south-side driver.

| Offset   | Register     | R/W | Notes                                                                         |
| -------- | ------------ | --- | ----------------------------------------------------------------------------- |
| `$FFA8`  | `FREQ_L`     | R/W | Frequency, low byte (see encoding)                                            |
| `$FFA9`  | `FREQ_H`     | R/W | Frequency, high byte                                                          |
| `$FFAA`  | `DUTY`       | R/W | Duty cycle, 0 (silent) – 255 (50 % square peak)                               |
| `$FFAB`  | *reserved*   | R/W | Currently unused                                                              |

**Frequency encoding.** The 16-bit value `FREQ = FREQ_H:FREQ_L` is mapped logarithmically to audio Hz:

$$
f(\text{FREQ}) = 20\,\text{Hz} \cdot 2^{10\,\text{FREQ}/65535}
$$

This covers roughly 20 Hz to 20 kHz across the 16-bit range. Writing either byte commits the new frequency. Writing `$FFAA` commits a new duty cycle independently.

---

## `$FFB0–$FFBF` — USB HID

USB keyboards, mice, and gamepads attached to the on-board ESP32-C3 USB host are exposed as a 16-byte window whose contents depend on which device is currently selected.

| Offset    | Register      | R/W | Notes                                                              |
| --------- | ------------- | --- | ------------------------------------------------------------------ |
| `$FFB0`   | `HID_SEL`     | W   | Device selector: `[3:0]` device type, `[7:4]` page / player index  |
| `$FFB1–$FFBF` | device data | R | Depends on selected device                                         |

**Device-type codes** (`HID_SEL[3:0]`):

| Code | Device    | High-nibble meaning                                                  |
| ---- | --------- | -------------------------------------------------------------------- |
| `0`  | Keyboard  | `0` or `1` — keyboard-state page (the full state is 32 bytes long)   |
| `1`  | Mouse     | Ignored                                                              |
| `2`  | Gamepad   | `0` — merged view (OR of all connected pads); `1`–`4` — pad 1–4      |

Writing to offset `$FFB0` commits the selector; the other fifteen bytes in the window are **read-only**.

### Keyboard (selector low nibble `0`)

The full keyboard state is 256 bits (32 bytes) — one bit per HID keycode. Bit `n` is set iff the key with HID keycode `n` is currently pressed. Because only 16 bytes are visible at a time, the window is split into two pages; set `HID_SEL` to `$00` to read bytes 0–15 of the state at `$FFB0–$FFBF`, and `$10` to read bytes 16–31.

The first byte (`$FFB0` when page 0 is selected) carries device status: `[0]` connected, `[1]` NUMLOCK LED, `[2]` CAPSLOCK LED, `[3]` SCROLLLOCK LED.

### Mouse (selector low nibble `1`)

| Offset     | Field           | Notes                                    |
| ---------- | --------------- | ---------------------------------------- |
| `$FFB1`    | Buttons         | `[0]` left · `[1]` right · `[2]` middle · `[3:7]` extended |
| `$FFB2`    | X delta (8-bit) | Signed                                   |
| `$FFB3`    | Y delta (8-bit) | Signed                                   |
| `$FFB4`    | Wheel           | Signed                                   |
| `$FFB5`    | Pan             | Signed horizontal wheel                  |
| `$FFB9–$FFBC` | X / Y counters | 16-bit absolute X and Y counters        |

### Gamepad (selector low nibble `2`)

Ten-byte snapshot for the selected pad (or for pad 0, the OR of all connected pads):

| Offset     | Field      | Notes                                                                            |
| ---------- | ---------- | -------------------------------------------------------------------------------- |
| `$FFB1`    | D-pad + features | `[0]` up · `[1]` down · `[2]` left · `[3]` right · `[6]` Sony layout · `[7]` gamepad valid |
| `$FFB2`    | Stick digitals    | `[3:0]` left-stick 4-direction · `[7:4]` right-stick 4-direction          |
| `$FFB3`    | Buttons 0         | Bits 0–7 of the button bitmap                                             |
| `$FFB4`    | Buttons 1         | Bits 8–15, including the Home button at bit 4                             |
| `$FFB5`    | Left stick X      | Signed 8-bit                                                              |
| `$FFB6`    | Left stick Y      | Signed 8-bit                                                              |
| `$FFB7`    | Right stick X     | Signed 8-bit                                                              |
| `$FFB8`    | Right stick Y     | Signed 8-bit                                                              |
| `$FFB9`    | Left trigger      | Unsigned 8-bit                                                            |
| `$FFBA`    | Right trigger     | Unsigned 8-bit                                                            |

The **merged-pad 0** view is the bitwise OR of all connected pads across every field. It is the right endpoint for single-player code that should accept input from any controller; multiplayer code should loop across pads 1–4.

---

## `$FFC0–$FFFF` — RIA

The RIA registers live at the very top of bank 0. They cover firmware status, the fastcall API at `$FFF0–$FFF3`, the native-mode 65816 vector table at `$FFE4–$FFFF`, and a handful of system services.

The 65816 native-mode vectors (reserved by the CPU) live at fixed offsets:

| Offset  | Vector              |
| ------- | ------------------- |
| `$FFE4` | COP (native)        |
| `$FFE6` | BRK (native)        |
| `$FFE8` | ABORTB (native)     |
| `$FFEA` | NMIB (native)       |
| `$FFEE` | IRQB (native)       |
| `$FFF4` | COP (emulation)     |
| `$FFF8` | ABORTB (emulation)  |
| `$FFFA` | NMIB (emulation)    |
| `$FFFC` | RESB                |
| `$FFFE` | IRQB / BRK (emulation) |

Because the X65 boots and operates exclusively in native mode, the emulation-mode vectors exist for completeness but are not used by X65 firmware or applications.

The fastcall window at `$FFF0–$FFF3` is the primary entry point for system calls. Arguments are passed through the 512-byte **XSTACK** maintained by the RIA. The full RIA register list is documented in the spreadsheet linked at the top of this appendix.
