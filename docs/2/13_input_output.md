# Chapter 13: Input/Output Handling

The I/O hardware and memory map are described in [Chapter 6](../1/6_io.md) and [Appendix A](../A/A_memory_map.md). This chapter is the programming-side counterpart — concrete assembly idioms for polling HID devices, driving LEDs and the buzzer, using the UART, and reaching the networking stack.

## Polling USB HID Gamepads

The HID window at **`$FFB0–$FFBF`** is device-switched: writing to `$FFB0` selects which device appears in the next fifteen bytes. For gamepads, the selector's low nibble is `2`; the high nibble picks the pad index (`0` = merged, `1..4` = physical pads 1..4).

### Merged pad 0 (single-player)

The simplest and the idiomatic starting point — read one virtual pad that is the bitwise OR of every connected controller:

```asm
    ; Select merged gamepad
    lda #$02                ; low nibble 2 = gamepad, high nibble 0 = merged
    sta $FFB0

    ; Direction buttons
    lda $FFB1               ; D-pad + features
    and #%00001000          ; bit 3 = right
    bne right_held

    lda $FFB1
    and #%00000100          ; bit 2 = left
    bne left_held

    ; Primary action button (bit 0 of buttons byte 0)
    lda $FFB3
    and #%00000001
    bne button_a_held
```

Bit 7 of `$FFB1` is the **valid** flag — `0` means no pad is connected. Single-player code should either treat an invalid pad as "no input" or use it to show a "connect a controller" screen.

### Multi-player

For multiplayer games, walk pads 1 through 4:

```asm
    ldx #1                  ; start at player 1
next_player:
    txa
    asl                     ; *= 2 (shift into high nibble)
    asl
    asl
    asl
    ora #$02                ; combine with gamepad type
    sta $FFB0               ; select that pad

    ; Read this pad's state...
    lda $FFB1
    and #%10000000
    beq skip_player         ; high bit clear = not connected
    ; ... process input for player X ...

skip_player:
    inx
    cpx #5
    bne next_player
```

Each pad's ten-byte report covers D-pad, digital-stick approximations, two button bytes, analog stick coordinates, and triggers — see [Appendix A](../A/A_memory_map.md) for the exact offsets.

### Analog sticks and deadzones

Analog stick bytes are signed 8-bit values. A reasonable deadzone filter:

```asm
    lda $FFB5               ; left stick X, signed
    sec
    sbc #-16                ; shift negative range up
    cmp #32                 ; below 16 in either direction = deadzone
    bcc inside_deadzone
    ; ... use stick value ...
inside_deadzone:
```

Tune the deadzone for the game; `±16` is a reasonable starting point for sticks that have seen some use.

## Polling USB HID Keyboards

Keyboard state is **256 bits** (one per HID keycode), exposed through the same window in two 16-byte pages. The low nibble of the selector is `0` (keyboard); the high nibble is `0` for bytes 0–15, `1` for bytes 16–31.

### "Any key pressed?"

Page 0, offset 0 is the device status byte. Bit 0 is connected, and the rest of the byte plus the next fifteen bytes reflect which keycodes are currently down. Fast existence check:

```asm
    lda #$00                ; keyboard, page 0
    sta $FFB0

    lda $FFB1
    ora $FFB2
    ora $FFB3
    ; ... through $FFBF ...
    beq no_key_pressed
```

For most games, a dedicated "any key" check is enough to dismiss a title screen.

### "Is this specific key down?"

Each state bit lives at bit `(keycode & 7)` of byte `(keycode >> 3) + 1` (offset 1, because byte 0 is the status byte). For HID keycode `$04` (letter A):

```asm
    lda #$00                ; page 0
    sta $FFB0
    lda $FFB1               ; byte 0 of state = status byte is there, bit 4 is "A"
    and #%00010000          ; bit 4
    bne a_key_down
```

For a keycode in page 1 (≥ 128), select page 1 first:

```asm
    lda #$10                ; keyboard, page 1
    sta $FFB0
    lda $FFB7               ; appropriate byte on page 1
    and #%00000100          ; the bit for the target key
    bne key_down
```

A small lookup table keyed by keycode that returns `(page, byte_offset, bit_mask)` is the usual way to make this readable from higher-level code.

### Modifier LEDs

Bits 1, 2, and 3 of the status byte carry the NUMLOCK, CAPSLOCK, and SCROLLLOCK LED states respectively. These are **host-computed** — the X65 firmware tracks them as the keyboard's HID output report. A program can use them to detect the modifier states without decoding the keycode-bitmap.

## DE-9 Joystick Ports

The on-board DE-9 joystick window at `$FF80–$FF97` is **currently stubbed** in firmware — reads return `$FF` and writes are ignored. Until the path from the PCAL6416A GPIO expander through the RIA interrupt controller is brought up, use USB HID gamepads via `$FFB0–$FFBF` for controller input.

Once the region goes live, the expected polling pattern is a small lookup against the expander's port registers, and the IRQ path (via the PCAL6416A's interrupt-mask registers) supports edge-driven input handling without continuous polling.

## RGB LED Programming

Two interfaces share `$FFA0–$FFA7`. Use whichever matches the application.

### Direct RGB332 — on-board LEDs 0..3

The simplest way to light one of the four direct-addressable LEDs. Format: `[7:5]R · [4:2]G · [1:0]B`. A single `STA` commits.

```asm
    lda #%11100000          ; pure red
    sta $FFA0               ; LED 0 → red

    lda #%00011100          ; pure green
    sta $FFA1               ; LED 1 → green

    lda #0                  ; off
    sta $FFA2               ; LED 2 → off
```

No setup, no sequencing, no need to touch any other register. Ideal for status indicators.

### Chain protocol — any LED 0..255, full 24-bit colour

The four bytes at `$FFA4–$FFA7` address an arbitrary LED in the WS2812 chain (including the on-board LEDs, which sit at the chain's low indices). The write to **`$FFA4` is the commit** — the other three bytes are just latched, so the order is: index, green, blue, **then red**.

```asm
    ; Light LED 5 (on the chain) bright red
    lda #5
    sta $FFA7               ; index — latched
    lda #$00
    sta $FFA5               ; G — latched
    lda #$00
    sta $FFA6               ; B — latched
    lda #$FF
    sta $FFA4               ; R — commits the update
```

To animate the chain, build a small table of (index, R, G, B) quads and loop over it — each quad is four stores.

## System Buzzer

Beeping the buzzer is the simplest I/O operation on the board: two writes to set frequency, one to set duty. Freq is encoded logarithmically (see [Chapter 6](../1/6_io.md)):

```asm
    ; ~1 kHz at 50% duty
    lda #$FF
    sta $FFA8               ; FREQ_L
    lda #$8C
    sta $FFA9               ; FREQ_H
    lda #$80
    sta $FFAA               ; DUTY
    ; ... wait ...
    lda #0
    sta $FFAA               ; silence
```

Practical pattern: keep a small lookup table from note index → 16-bit FREQ code, then `ldx note; lda table_lo,x; sta $FFA8; lda table_hi,x; sta $FFA9` to start a tone; set DUTY to 0 to stop.

## UART / Monitor Console from a Program

The UART pair at `$FFE0–$FFE1` gives a program direct access to the same serial path that the monitor uses. With nothing special to configure, a program can print to the host terminal over USB-CDC or talk to another serial device.

### Transmit

```asm
tx_byte:                    ; A = byte to send
    pha
tx_wait:
    lda $FFE0               ; status
    and #%10000000          ; bit 7 = TX writable
    beq tx_wait
    pla
    sta $FFE1               ; write the byte
    rts
```

Sending a zero-terminated string:

```asm
puts:                       ; X:Y = string pointer (cc65 convention)
    ; ... loop: lda (str),y; beq done; jsr tx_byte; iny; bne loop; done: rts ...
```

### Receive

```asm
rx_byte:                    ; return byte in A, or carry set if none pending
    lda $FFE0               ; status
    and #%01000000          ; bit 6 = RX ready
    beq no_rx
    lda $FFE1               ; read the byte (also clears the ready flag)
    clc
    rts
no_rx:
    sec
    rts
```

A polling program reads one byte per VBI or main-loop iteration.

## Summary

The X65's I/O surface is uniformly memory-mapped and uniformly poll-or-IRQ-driven. HID gamepad and keyboard support is a single selector-plus-read pattern per device; LEDs and the buzzer are one-or-a-few stores; the UART is a classic two-register status-plus-data flow. Register offsets and bit layouts are in [Appendix A](../A/A_memory_map.md); the hardware context is in [Chapter 6](../1/6_io.md).
