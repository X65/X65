# Appendix E: X65 Quick Reference Cheat Sheet

## 65816 CPU Registers

| Register | Size     | Description           | Notes                                     |
| -------- | -------- | --------------------- | ----------------------------------------- |
| A (,B=C) | 8/16-bit | Accumulator           | Width controlled by m flag (0=16-bit)     |
| X        | 8/16-bit | X Index Register      | Width controlled by x flag (0=16-bit)     |
| Y        | 8/16-bit | Y Index Register      | Width controlled by x flag (0=16-bit)     |
| S        | 16-bit   | Stack Pointer         | Points to next free location on stack     |
| D        | 16-bit   | Direct Page Register  | Base address for direct page addressing   |
| PBR      | 8-bit    | Program Bank Register | Current code bank (upper 8 bits of PC)    |
| DBR      | 8-bit    | Data Bank Register    | Default data bank for absolute addressing |
| P        | 8-bit    | Processor Status      | Contains status flags                     |

## Status Register (P) Flags

| Bit | Flag | Description               | When Set (1)            | When Clear (0)         |
| --- | ---- | ------------------------- | ----------------------- | ---------------------- |
| 7   | N    | Negative                  | Result is negative      | Result is positive     |
| 6   | V    | Overflow                  | Overflow occurred       | No overflow            |
| 5   | M    | Memory/Accumulator Select | 8-bit accumulator       | 16-bit accumulator     |
| 4   | X    | Index Register Select     | 8-bit index registers   | 16-bit index registers |
| 3   | D    | Decimal Mode              | BCD arithmetic mode     | Binary arithmetic mode |
| 2   | I    | IRQ Disable               | IRQ interrupts disabled | IRQ interrupts enabled |
| 1   | Z    | Zero                      | Result is zero          | Result is non-zero     |
| 0   | C    | Carry                     | Carry occurred          | No carry occurred      |

## Common 65816 Instructions

### Register Instructions

```asm
REP #$30      ; Reset bits in P (set 16-bit A,X,Y mode)
SEP #$30      ; Set bits in P (set 8-bit A,X,Y mode)
XBA           ; Exchange B and A (swap high/low bytes)
TCS           ; Transfer C (16-bit A) to S
TCD           ; Transfer C (16-bit A) to D
PHB/PLB       ; Push/Pull Data Bank Register
PHD/PLD       ; Push/Pull Direct Page Register
PHK           ; Push Program Bank Register
```

### Mode Switching

```asm
CLC           ; Clear carry
XCE           ; Exchange Carry with Emulation flag (enter native mode)
SEC           ; Set carry
XCE           ; Exchange Carry with Emulation flag (enter emulation mode)
```

### Bank Register Management

```asm
LDA #$01      ; Load bank number
PHA           ; Push to stack
PLB           ; Pull to Data Bank Register
```

### Memory Block Operations

```asm
MVN src,dst   ; Move memory block (ascending)
MVP src,dst   ; Move memory block (descending)
```

### 16-bit Operations

```asm
REP #$20      ; 16-bit accumulator mode
LDA #$1234    ; Load 16-bit immediate value
ADC #$4321    ; 16-bit addition
SBC #$0001    ; 16-bit subtraction
```

### Long Addressing

```asm
JML $123456   ; Jump Long (sets Program Bank Register)
JSL $123456   ; Jump to Subroutine Long
RTL           ; Return from Subroutine Long
LDA $123456   ; Load from long address
STA $123456   ; Store to long address
```

## Common Assembly Code Snippets

### System Initialization

```asm
    CLC                     ; Clear carry flag
    XCE                     ; Switch to native mode
    REP #$30                ; 16-bit A,X,Y
    LDA #$0000              ; Set direct page to 0
    TCD
    LDX #$01FF              ; Set up stack at $01FF
    TXS
    SEP #$20                ; 8-bit A for I/O operations
```

### Playing a Note on the SGU-1

The SGU-1 register window lives at `$FEC0-$FEFF` and is channel-switched: `$FEFF` selects which channel the
window exposes. Within the window, `$FEC0-$FEDF` are the four operators (8 bytes each) and `$FEE0-$FEFE` are
the channel-wide controls. See [Chapter 12](../2/12_sound.md) for the full sequence and
[Appendix G](G_sgu1.md) for what the fields mean.

```asm
    SEP #$30                ; 8-bit A *and* index registers
    LDY #8                  ; reset: wipe all 9 channels to zero
RESET_CH:
    STY $FEFF               ; select the channel
    LDX #$3E
    LDA #$00
RESET_REG:
    STA $FEC0,X
    DEX
    BPL RESET_REG
    DEY
    BPL RESET_CH

    LDA #$FF                ; the chip powers up muted
    STA $FEFF               ; select the service bank
    LDA #$FF
    STA $FEE0               ; master volume: unmute

    LDA #$00
    STA $FEFF               ; select channel 0

    LDA #<7382              ; phase increment for A4 (440 Hz)
    STA $FEE0               ; SGU CHN_FREQ_L
    LDA #>7382
    STA $FEE1               ; SGU CHN_FREQ_H

    LDA #64                 ; volume
    STA $FEE2               ; SGU CHN_VOL

    LDA #$03                ; FLAGS0: GATE (bit 0) + TRIG (bit 1) = note-on
    STA $FEE4               ; SGU CHN_FLAGS0
```

`FREQ` is a phase increment, not a frequency: multiply hertz by 16.777 (`FREQ = Hz * 2^24 / 1000000`).

## CGIA Register Reference

The CGIA register window lives at `$FF00–$FF7F`. Frequently-touched registers:

| Address       | Register      | Notes                                                                                          |
| ------------- | ------------- | ---------------------------------------------------------------------------------------------- |
| `$FF00`       | `mode`        | Bit 0 = HIRES (768 px horizontal), bit 1 = INTERLACE (480 vert).                               |
| `$FF01`       | `bckgnd_bank` | High 8 bits of address for background-plane fetches.                                           |
| `$FF02`       | `sprite_bank` | High 8 bits of address for sprite fetches.                                                     |
| `$FF10–$FF11` | `raster`      | Current raster line (read-only).                                                               |
| `$FF12`       | `rst_status`  | Raster status bits.                                                                            |
| `$FF18–$FF19` | `int_raster`  | Line at which to fire a raster interrupt.                                                      |
| `$FF1A`       | `int_enable`  | `[VBI DLI RSI x x x x x]`                                                                      |
| `$FF1B`       | `int_status`  | Same layout; write to acknowledge.                                                             |
| `$FF30`       | `planes`      | High nibble = plane type (0 background, 1 sprite); low nibble = enable bits per plane.         |
| `$FF31`       | `order`       | Plane Z-order permutation (one byte selects one of 24 SJT-encoded orderings).                  |
| `$FF34`       | `back_color`  | Border / fill color.                                                                           |
| `$FF38–$FF3F` | `offset[4]`   | 16-bit per-plane pointers to the display list or sprite descriptor table.                      |
| `$FF40–$FF7F` | `plane[0..3]` | Four blocks of 16 plane registers each (interpretation depends on plane type and active mode). |

## USB HID Quick Reference

The HID window at `$FFB0–$FFBF` is rebindable: writing to `$FFB0` selects which device (and, for keyboards, which page or, for gamepads, which player) appears in `$FFB1–$FFBF`.

### Polling a Gamepad

```asm
    ; Select merged gamepad (OR of all connected pads). Use $12/$22/$32/$42
    ; to select specific player 1..4 instead.
    LDA #$02
    STA $FFB0

    LDA $FFB1            ; D-pad (bits 0..3) + features (bits 6..7)
    LDA $FFB3            ; buttons 0..7
    LDA $FFB4            ; buttons 8..15 (Home = bit 4)
    LDA $FFB5            ; left-stick X  (signed 8-bit)
    LDA $FFB6            ; left-stick Y
    LDA $FFB9            ; left trigger (unsigned 8-bit)
```

### Polling a USB Keyboard

```asm
    ; Full keyboard state is 32 bytes, exposed in two 16-byte pages.
    ; Page 0 (high-nibble 0): bytes 0..15 of the state bitmap.
    ; Page 1 (high-nibble 1): bytes 16..31.
    LDA #$00              ; keyboard, page 0
    STA $FFB0

    ; "any key pressed?" — BIT on the "valid device" flag at $FFB0
    BIT $FFB0             ; N/Z reflect bits 7/6 of the first byte

    ; "is key with HID keycode $K pressed?"
    ; Each state bit lives at bit (K & 7) of byte (K >> 3).
    ; Example: HID keycode $04 (letter A) → bit 4 of byte 0 of page 0.
    LDA $FFB1             ; byte 0 of state
    AND #%00010000        ; mask bit 4 (A key)
    BNE a_key_down
```

## RGB LED Quick Reference

The LED window at `$FFA0–$FFA7` has two parallel interfaces: four direct RGB332 bytes for the on-board LEDs (three are populated on the DEV-board), and a four-byte chain protocol for addressing any LED in a WS2812 chain.

### Direct RGB332 (LEDs 0–3)

```asm
    ; RGB332 byte: [7:5]R, [4:2]G, [1:0]B
    LDA #%11100000        ; pure red
    STA $FFA0             ; LED 0 → red (single STA commits)

    LDA #%00011100        ; pure green
    STA $FFA1             ; LED 1 → green
```

### Chain Protocol (any LED 0–255, 24-bit RGB)

The write to `$FFA4` is the commit — latch the R, G and B bytes first, then store the LED index last:

```asm
    LDA #$FF              ; red
    STA $FFA5             ; latched
    LDA #$80              ; green
    STA $FFA6             ; latched
    LDA #$00              ; blue
    STA $FFA7             ; latched
    LDA #5                ; target LED index → this write commits the chain update
    STA $FFA4
```

## System Buzzer Quick Reference

The buzzer window lives at `$FFA8–$FFAB`. Frequency is encoded logarithmically as `FREQ_16bit / 65535 × 10` octaves starting from 20 Hz.

```asm
    ; ~1 kHz beep at 50% duty
    LDA #$FF              ; FREQ_L
    STA $FFA8
    LDA #$8C              ; FREQ_H
    STA $FFA9
    LDA #$80              ; 50% duty
    STA $FFAA

    ; Silence
    LDA #0
    STA $FFAA             ; duty = 0 → no output
```

## DE-9 GPIO Quick Reference

Two DE-9 ports of bi-directional 5 V GPIO (PCAL6416A) at `$FF80–$FF97`; port 0 = first connector, port 1 = second. Bits 0–7 map to DE-9 pins 1–7, 9 (pin 8 is ground); joystick convention: Up/Down/Left/Right = bits 0–3, fire = bit 5, buttons 2/3/4 = bits 7/4/6, all active-low. *Not decoded by firmware yet — reads return `$FF`.*

```asm
    ; Joystick setup: enable pull-ups (pins reset as inputs, PLS defaults to pull-up)
    LDA #$FF
    STA $FF8E             ; PLE0 — port 0 pulls on
    ; Poll (0 = pressed)
    LDA $FF80             ; IN0
    AND #%00100000        ; bit 5 = fire
    BEQ fire_held

    ; GPIO output: level first, then direction
    LDA #%00000001
    STA $FF82             ; OUT0 — drive bit 0 high
    LDA #%11111110
    STA $FF86             ; CFG0 — 0 = output

    ; Edge IRQ: unmask pin, handler reads INST0 then IN0 to clear
    LDA #%11111110
    STA $FF92             ; INTE0 — 0 = IRQ on change (RIA IRQ bit 1)
```

Full register semantics: [Appendix A](A_memory_map.md), [Chapter 6](../1/6_io.md).

## System Timers Quick Reference

Two CIA-compatible 16-bit timers at `$FF98–$FF9F`, each counting down in 1 µs ticks.

```asm
    ; Set Timer A to 1 ms (1000 µs), continuous, enable its interrupt
    LDA #<1000
    STA $FF98             ; TAL — latches low byte
    LDA #>1000
    STA $FF99             ; TAH — loads latched value into counter
    LDA #%10000001        ; [7]=1 set bits, [0]=1 enable Timer A IRQ
    STA $FF9D             ; ICR
    LDA #%00000001        ; [0]=1 start Timer A (continuous mode)
    STA $FF9E             ; CRA

    ; Read counter (µs remaining)
    LDA $FF98             ; low  byte
    LDX $FF99             ; high byte
```

Reading `ICR` ($FF9D) clears the pending interrupt flags; latch it into a register before dispatching.
