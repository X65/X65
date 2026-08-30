# Appendix A: Memory Map

The X65 exposes a flat 24-bit address space to the 65C816. Most of that space is plain PSRAM; a 512-byte window at the top of bank 0 is carved up into memory-mapped-I/O regions owned by the custom chips, and a small expansion window sits slightly below it. This appendix documents every byte of that MMIO region.

The canonical live spreadsheet is at <https://tinyurl.com/x65-memory-map>:

<iframe width="100%" height="600" src="https://docs.google.com/spreadsheets/d/e/2PACX-1vRL2pdhGaOKfOaP7AGEpHNyDYzaYB4PVj7eJQSQAT52y95yiliqZ90WcLBRTPOZ93MihAFuKKmyBL1n/pubhtml?gid=0&amp;single=true&amp;widget=true&amp;headers=false"></iframe>

The tables below mirror that spreadsheet for offline and search-engine-friendly reference.

## Top-Level MMIO Layout

Bank 0, pages `$FC`–`$FF`:

| Range         | Owner                               | Notes                                                  |
| ------------- | ----------------------------------- | ------------------------------------------------------ |
| `$FC00–$FDFF` | Expansion slots                     | 4 cards × 128 bytes                                    |
| `$FEC0–$FEFF` | SGU-1 (sound)                       | 64-byte channel-switched window                        |
| `$FF00–$FF7F` | CGIA (graphics)                     | 128 registers                                          |
| `$FF80–$FF97` | GPIO expander / joystick            | PCAL6416A, 2× DE-9 (not decoded by firmware yet)       |
| `$FF98–$FF9F` | System timers                       | Two CIA-compatible 16-bit counters, 1 µs resolution    |
| `$FFA0–$FFA7` | RGB LED chain                       | 4 direct RGB332 + 4-byte chain protocol                |
| `$FFA8–$FFAB` | System buzzer                       | 16-bit log frequency + 8-bit duty                      |
| `$FFAC–$FFAF` | Reserved                            | Reads return `$FF`                                     |
| `$FFB0–$FFBF` | USB HID (keyboard / mouse / gamepad)| Device-selector at `$FFB0`                             |
| `$FFC0–$FFFF` | RIA                                 | Math, TOD, DMA, files, UART, IRQ, fastcall API, CPU vectors |

Everything outside the top half-page (`$FExx–$FFxx`) and the expansion window at `$FCxx–$FDxx` is plain PSRAM and available to software; chunks of the expansion window can be reclaimed as PSRAM through `EXTIOCTL` (see below). Bank crossing happens on the fly at `$800000`.

---

## `$FC00–$FDFF` — Expansion Slots

The expansion window is **512 bytes**, divided into **four 128-byte slots** — one per card. The expansion port routes four `IO_EN` signals and four `IO_INT` signals, and a card claims its slot through its own `IO_EN` line:

| Range         | Slot | Enable   |
| ------------- | ---- | -------- |
| `$FC00–$FC7F` | 0    | `IO0_EN` |
| `$FC80–$FCFF` | 1    | `IO1_EN` |
| `$FD00–$FD7F` | 2    | `IO2_EN` |
| `$FD80–$FDFF` | 3    | `IO3_EN` |

The whole window belongs to the expansion bus by default. Few peripherals have any use for 128 registers, though, so the RIA can hand parts of it back: **`EXTIOCTL` (`$FFF6`)** is a bitmap of **eight 64-byte chunks**, two per slot, and setting a bit maps that chunk back to **RAM**. A card that only needs 64 bytes therefore need not cost the program the other 64.

| Bit | Chunk range   | Slot |
| --- | ------------- | ---- |
| 0   | `$FC00–$FC3F` | 0    |
| 1   | `$FC40–$FC7F` | 0    |
| 2   | `$FC80–$FCBF` | 1    |
| 3   | `$FCC0–$FCFF` | 1    |
| 4   | `$FD00–$FD3F` | 2    |
| 5   | `$FD40–$FD7F` | 2    |
| 6   | `$FD80–$FDBF` | 3    |
| 7   | `$FDC0–$FDFF` | 3    |

`EXTIOCTL` powers up as `$00`, so a freshly started machine has the entire `$FC00–$FDFF` window on the expansion bus and none of it as memory. Reading a chunk that is on the bus with no card answering returns `$FF`; writes to it go nowhere.

No register layout is imposed by the core system — each expansion board defines its own map within its slot, and a board is free to use only the first few bytes of it. See [Chapter 6: Input and Output Interfaces](../1/6_io.md) for the expansion port pinout.

---

## `$FEC0–$FEFF` — SGU-1

SGU-1 presents a **single 64-byte window** that is re-bound to a specific bank by writing a selector to the last byte. Values `$00`–`$08` select the nine synthesis channels. `$FF` selects the **service bank**, which reaches past the synthesis engine: PCM sample data is uploaded through it, and the master volume there is a proxy for the CODEC/DSP downstream of the waveform generator. Everything in between (`$09`–`$FE`) is reserved: writes are ignored and reads return `$FF`. The selector is stored verbatim; out-of-range values are **not** wrapped onto a channel.

**SGU-1 comes up muted.** The service bank's master volume gates the entire mix and resets to `0`, so the chip is silent until software raises it. This is deliberate: a reset must never blast the user with whatever the channel register file happened to power up holding. Put the channels into a known state first, then unmute. Under OS/816 that is the system's job; a bare-metal program has to do it itself, or it will hear nothing.

The selector:

| Offset         | Register      | R/W | Notes                                                          |
| -------------- | ------------- | --- | -------------------------------------------------------------- |
| `$3F` (`$FEFF`)| `CH_SELECT`   | R/W | Write: select bank — `$00`–`$08` channel, `$FF` service bank, `$09`–`$FE` reserved. Read: returns the value last written. Reset value `$00`. |

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
| `$24`  | `FLAGS0`         | `[0] GATE` (key level: a rise out of release attacks from the current attenuation; a sounding envelope is left alone), `[1] TRIG` (one-shot, self-clearing: restarts the envelope from silence), `[3] PCM`, `[4] RING_MOD`, `[5] NSLOW` / `[6] NSHIGH` / `[7] NSBAND` filter output selects |
| `$25`  | `FLAGS1`         | Phase reset, filter reset, PCM loop, per-sweep enables, `[7] DIAG` diagnostic-readback switch |
| `$26`  | `CUTOFF_L`       | Filter cutoff, low byte                                                     |
| `$27`  | `CUTOFF_H`       | Filter cutoff, high byte                                                    |
| `$28`  | `DUTY`           | Pulse width, **signed** (`int8`): the magnitude is the LOW-run length out of 128; the sign places that run at the start (positive) or the end (negative) of the period |
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
| `$3E`  | `LFOW`           | LFO shapes: `[3:2] PM_SHAPE`, `[1:0] AM_SHAPE` — `0` saw, `1` square, `2` triangle, `3` noise |
| `$3F`  | `CH_SELECT`      | Bank selector (see above)                                                   |

PCM sample data itself lives in 64 KB of RAM internal to the audio chip, addressed via the `PCM_POS` / `PCM_END` / `PCM_RST` pointers; it is **not** visible in the 65816's address space.

Writes to this window are intercepted by NORTH and forwarded to the audio chip via the `SPU` device on the PIX bus (see [Chapter 2: System Architecture Overview](../1/2_overview.md)). The SGU-1 is a self-contained module hosted on the board and bridged from SOUTH over SPI; the SOUTH-side driver caches register values so repeat reads avoid a round-trip.

---

## `$FF00–$FF7F` — CGIA

CGIA exposes **128 registers**. The control block occupies the first 64; the upper 64 are four per-plane banks.

| Address       | Register      | R/W | Notes                                                                             |
| ------------- | ------------- | --- | --------------------------------------------------------------------------------- |
| `$FF00`       | `MODE`        | R/W | `[0] HIRES` (96-column / 768 px horizontal) · `[1] INTERLACE` (480-line vertical) |
| `$FF01`       | `BCKGND_BANK` | R/W | Bank number for background-plane fetches                                          |
| `$FF02`       | `SPRITE_BANK` | R/W | Bank number for sprite fetches                                                    |
| `$FF10–$FF11` | `RASTER`      | R   | Current raster line (16-bit)                                                      |
| `$FF12`       | `RST_STATUS`  | R   | Raster status bits                                                                |
| `$FF18–$FF19` | `INT_RASTER`  | R/W | Line at which to fire a raster interrupt (16-bit)                                 |
| `$FF1A`       | `INT_ENABLE`  | R/W | `[7] VBI · [6] DLI · [5] RSI`                                                     |
| `$FF1B`       | `INT_STATUS`  | R/W | Same layout; write to acknowledge                                                 |
| `$FF30`       | `PLANES`      | R/W | `[7:4]` plane type (0 background, 1 sprite) · `[3:0]` per-plane enable            |
| `$FF31`       | `ORDER`       | R/W | Encodes one of 24 Z-order permutations of the four planes (Steinhaus-Johnson-Trotter) |
| `$FF34`       | `BACK_COLOR`  | R/W | Backdrop / border colour                                                          |
| `$FF38–$FF39` | `OFFSET0`     | R/W | Plane 0 display-list or sprite-descriptor table start (16-bit)                    |
| `$FF3A–$FF3B` | `OFFSET1`     | R/W | Plane 1 table start                                                               |
| `$FF3C–$FF3D` | `OFFSET2`     | R/W | Plane 2 table start                                                               |
| `$FF3E–$FF3F` | `OFFSET3`     | R/W | Plane 3 table start                                                               |
| `$FF40–$FF4F` | `PLANE0[16]`  | R/W | Plane 0 registers — interpretation depends on plane type and active mode          |
| `$FF50–$FF5F` | `PLANE1[16]`  | R/W | Plane 1 registers                                                                 |
| `$FF60–$FF6F` | `PLANE2[16]`  | R/W | Plane 2 registers                                                                 |
| `$FF70–$FF7F` | `PLANE3[16]`  | R/W | Plane 3 registers                                                                 |

Addresses not listed inside `$FF00–$FF3F` are reserved. For the per-plane register layouts see [Chapter 4: Graphics and Display](../1/4_graphics.md) and [Chapter 11: Graphics Programming](../2/11_graphics.md) for the plane-register map, display-list instruction encoding, and sprite-descriptor format.

---

## `$FF80–$FF97` — GPIO Expander

This 24-byte window maps the on-board **PCAL6416A** I²C GPIO expander that routes the two DE-9 joystick ports — port 0 and port 1 of the expander, one per connector (DE-9 pin 8 is ground). Registers pair up as `xx0` for port 0 and `xx1` for port 1:

| Address           | Register        | Notes                                                              |
| ----------------- | --------------- | ------------------------------------------------------------------ |
| `$FF80` / `$FF81` | `IN0` / `IN1`   | Input port — current pin levels                                    |
| `$FF82` / `$FF83` | `OUT0` / `OUT1` | Output port                                                        |
| `$FF84` / `$FF85` | `POL0` / `POL1` | Polarity inversion                                                 |
| `$FF86` / `$FF87` | `CFG0` / `CFG1` | Configuration — 1 = input, 0 = output                              |
| `$FF88`–`$FF8B`   | `STR0L`/`STR0H`/`STR1L`/`STR1H` | Output drive strength, two bits per pin        |
| `$FF8C` / `$FF8D` | `LTCH0` / `LTCH1` | Input latch                                                      |
| `$FF8E` / `$FF8F` | `PLE0` / `PLE1` | Pull-up / pull-down enable                                         |
| `$FF90` / `$FF91` | `PLS0` / `PLS1` | Pull-up / pull-down selection                                      |
| `$FF92` / `$FF93` | `INTE0` / `INTE1` | Interrupt mask — clear a bit to request an IRQ on that pin        |
| `$FF94` / `$FF95` | `INST0` / `INST1` | Interrupt status                                                 |
| `$FF96`           | —               | Reserved                                                           |
| `$FF97`           | `OUTCF`         | Output port configuration (push-pull / open-drain)                 |

The interrupt-mask registers are the expander's defining feature for the X65: software asks for IRQs only on the pin transitions it cares about, instead of re-reading every pin on every change.

:::{note}
The NORTH firmware does not decode this window yet — reads currently return `$FF` and writes are ignored. Until it is activated, use USB HID gamepads via `$FFB0–$FFBF`.
:::

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

The RIA decodes four direct LED registers and the firmware drives all four (`RGB_LED_COUNT 4`), but the X65 DEV-board populates only the first **three** WS2812B-style RGB LEDs; `$FFA3` addresses a fourth position you can populate yourself on the strip header. A chain of up to 256 LEDs is supported via the expansion port's `WS2812` data line.

| Offset   | Register      | R/W | Notes                                                         |
| -------- | ------------- | --- | ------------------------------------------------------------- |
| `$FFA0`  | `LED0`        | R/W | Direct RGB332 colour for LED 0; write commits immediately     |
| `$FFA1`  | `LED1`        | R/W | Direct RGB332 colour for LED 1                                |
| `$FFA2`  | `LED2`        | R/W | Direct RGB332 colour for LED 2                                |
| `$FFA3`  | `LED3`        | R/W | Direct RGB332 colour for LED 3 (position unpopulated on the DEV-board) |
| `$FFA4`  | `LED_IDX`     | R/W | LED index in the chain (0–255); **writing here commits the chain update** |
| `$FFA5`  | `LED_RED`     | R/W | Red byte (0–255); latched                                     |
| `$FFA6`  | `LED_GREEN`   | R/W | Green byte (0–255); latched                                   |
| `$FFA7`  | `LED_BLUE`    | R/W | Blue byte (0–255); latched                                    |

**RGB332 byte** (`$FFA0–$FFA3`): `[7:5] R` · `[4:2] G` · `[1:0] B`. A single `STA $FFA0` sets LED 0 — the LED 0–3 interface is one instruction per LED.

**Chain protocol** (`$FFA4–$FFA7`): to set LED *n* to 24-bit colour, latch the red, green and blue bytes into `$FFA5`, `$FFA6` and `$FFA7`, then write the LED index to `$FFA4`. The write to `$FFA4` is what dispatches the update to the hardware; the other three bytes are simply latched. (Order of the latch writes is free; only the write to `$FFA4` must be last.)

---

## `$FFA8–$FFAB` — System Buzzer

A PWM-driven piezo buzzer. Two commands are exposed to the CPU; each write forwards a PIX command to the south-side driver.

| Offset   | Register     | R/W | Notes                                                                         |
| -------- | ------------ | --- | ----------------------------------------------------------------------------- |
| `$FFA8`  | `BUZZ_FREQ_LO` | R/W | Frequency, low byte (see encoding)                                          |
| `$FFA9`  | `BUZZ_FREQ_HI` | R/W | Frequency, high byte                                                        |
| `$FFAA`  | `BUZZ_DUTY`    | R/W | Duty cycle, 0 (silent) – 255 (50 % square peak)                             |
| `$FFAB`  | `BUZZ_RES`   | R/W | Reserved; currently unused                                                    |

**Frequency encoding.** The 16-bit value `FREQ = BUZZ_FREQ_HI:BUZZ_FREQ_LO` is mapped logarithmically to audio Hz:

$$
f(\text{FREQ}) = 20\,\text{Hz} \cdot 2^{10\,\text{FREQ}/65535}
$$

This covers roughly 20 Hz to 20 kHz across the 16-bit range. Writing either byte commits the new frequency. Writing `$FFAA` commits a new duty cycle independently.

---

## `$FFB0–$FFBF` — USB HID

USB keyboards, mice, and gamepads attached to the NORTH chip's USB host stack are exposed as a 16-byte window whose contents depend on which device is currently selected.

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

The RIA registers live at the very top of bank 0. The 65C816 reserves two vector tables up here — native at `$FFE4–$FFEF` and emulation at `$FFF4–$FFFF` — and the RIA fills the gaps the CPU leaves in and around them with system services: hardware multiply/divide, a time-of-day counter, DMA, file descriptors, the UART, an RNG, the interrupt controller and the fastcall API.

### Hardware Multiply and Divide

| Address       | Register | R/W | Notes                                              |
| ------------- | -------- | --- | -------------------------------------------------- |
| `$FFC0–$FFC1` | `OPERA`  | R/W | Operand A (16-bit)                                 |
| `$FFC2–$FFC3` | `OPERB`  | R/W | Operand B (16-bit)                                 |
| `$FFC4–$FFC7` | `MULAB`  | R   | `OPERA × OPERB` (32-bit product)                   |
| `$FFC8–$FFC9` | `DIVAB`  | R   | Signed `OPERA` ÷ unsigned `OPERB` (16-bit quotient) |

Both results are computed combinatorially from the current operands — write the operands, then read the result; there is no "start" or "busy" handshake. Division by zero yields `$FFFF`.

### Time of Day

| Address       | Register    | R/W | Notes                                          |
| ------------- | ----------- | --- | ---------------------------------------------- |
| `$FFCA–$FFCF` | `TM0`–`TM5` | R   | Monotonic microseconds since boot (48-bit, little-endian) |

### DMA

| Address       | Register   | R/W | Notes                             |
| ------------- | ---------- | --- | --------------------------------- |
| `$FFD0–$FFD2` | `ADDRSRC`  | R/W | Source address (24-bit)           |
| `$FFD3`       | `STEPSRC`  | R/W | Source step                       |
| `$FFD4–$FFD6` | `ADDRDST`  | R/W | Destination address (24-bit)      |
| `$FFD7`       | `STEPDST`  | R/W | Destination step                  |
| `$FFD8`       | `COUNT`    | R/W | Transfer count                    |
| `$FFD9`       | `DMAERR`   | R   | Transfer `errno`                  |

### File Descriptors

| Address | Register | R/W | Notes                                                          |
| ------- | -------- | --- | --------------------------------------------------------------- |
| `$FFDA` | `FDA`    | R/W | File-descriptor A number (obtained from the `open()` API call)  |
| `$FFDB` | `FDB`    | R/W | File-descriptor B number                                        |
| `$FFDC` | `FDARW`  | R/W | Read a byte from FDA / write a byte to FDA                      |
| `$FFDD` | `FDBRW`  | R/W | Read a byte from FDB / write a byte to FDB                      |
| `$FFDE` | `FDAST`  | R   | File-descriptor A status                                        |
| `$FFDF` | `FDBST`  | R   | File-descriptor B status                                        |

Streaming a file is a tight loop on `FDARW` — no API round-trip per byte.

### UART, RNG and Interrupt Controller

| Address       | Register     | R/W | Notes                                                     |
| ------------- | ------------ | --- | ---------------------------------------------------------- |
| `$FFE0`       | `READY`      | R   | UART FIFO flow control — TX-ready and RX-available flags   |
| `$FFE1`       | `TX` / `RX`  | R/W | Write to transmit, read to receive                         |
| `$FFE2–$FFE3` | `RNG`        | R   | Random number generator; two bytes so 16-bit values can be read at once |
| `$FFEC`       | `IRQ_ENABLE` | R/W | RIA interrupt enable mask                                  |
| `$FFED`       | `IRQ_STATUS` | R   | Interrupt controller status — which source raised `IRQB`   |

`IRQ_ENABLE` and `IRQ_STATUS` sit in the two bytes the 65C816 leaves reserved inside the native vector table, and `EXTIOCTL` / `EXTMEM` (below) occupy the matching reserved pair in the emulation table.

### Fastcall API

| Address       | Register  | R/W | Notes                                                                 |
| ------------- | --------- | --- | ---------------------------------------------------------------------- |
| `$FFF0`       | `OP`/`RET`| R/W | Write the API operation id to begin a kernel call; read the return value |
| `$FFF1`       | `RET_HI`  | R   | High byte of a 16-bit return value, otherwise `0`                      |
| `$FFF2`       | `STACK`   | R/W | XSTACK port — 512 bytes for passing call parameters                    |
| `$FFF3`       | `STATUS`  | R   | `[7]` high while the operation is running; `[0]` high when `ERRNO` is set |

### Extension Control

| Address | Register   | R/W | Notes                                                                   |
| ------- | ---------- | --- | ------------------------------------------------------------------------ |
| `$FFF6` | `EXTIOCTL` | R/W | Bitmap of the eight 64-byte chunks of `$FC00–$FDFF` (see the expansion section above) |
| `$FFF7` | `EXTMEM`   | R/W | Reserved for future use (extended-memory MMU)                            |

### CPU Vectors

The 65C816 vectors (reserved by the CPU) live at fixed offsets:

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

The fastcall window at `$FFF0–$FFF3` is the primary entry point for system calls. Arguments are passed through the 512-byte **XSTACK** maintained by the RIA.
