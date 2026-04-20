# Chapter 12: Sound Programming

The SGU-1 synthesis model and the audio plumbing are covered in [Chapter 5](../1/5_audio.md); the full register map lives in [Appendix A](../A/A_memory_map.md). This chapter is the programming-side counterpart — the idiomatic sequences for picking a channel, setting up a voice, running an envelope, sweeping a parameter, playing back PCM, and beeping the buzzer.

## The SGU-1 Register Window

The whole of the SGU-1 is reached through a **64-byte window at `$FEC0–$FEFF`**. That window is **channel-switched**: the byte at offset `$3F` (i.e. `$FEFF`) is a selector that remaps the other 63 bytes to the registers of one specific channel. Writing a channel number `0..8` there switches the window; writes wrap modulo-9, so `STA $FEFF` with an invalid value simply lands on channel (value mod 9).

A minimal programming cycle therefore looks like: select channel, configure, key on. To configure a different channel, select it and configure again. Changes to one channel do not touch the others' state.

## Selecting a Channel

```asm
    lda #0                  ; channel 0
    sta $FEFF               ; SGU channel-select
```

After this, all reads and writes to `$FEC0–$FEFE` apply to channel 0. The selector itself is stateful on the audio side — the next write to `$FEFF` switches; no other operation invalidates the selection.

## The Channel Layout, At a Glance

The 63 programmable bytes fall into two halves:

- **`$FEC0–$FEDF`** — four 8-byte **operators** (numbered 0..3).
- **`$FEE0–$FEFE`** — **channel-wide** controls: frequency, volume, pan, filter, sweeps, PCM pointers, flags.

Per-operator registers (`R0..R7`) hold everything needed for one FM operator: waveform, multiplier, detune, envelope rates and levels, routing, and the small handful of ESFM-style extras (fixed-frequency flag, delayed key-on, ring mod, hard sync). See [Appendix A](../A/A_memory_map.md) for bit-level layout.

## Programming a Voice

The order of operations matters less than you might expect — almost every register is latched and only evaluated when the channel's **GATE** bit is set. The canonical setup sequence:

1. Select the channel.
2. Set per-operator fields: waveform, `MUL`, `TL`, envelope (`AR` / `DR` / `SL` / `RR` / `SR`), routing (`OUT` / `MOD`).
3. Set channel fields: pitch (`FREQ`), volume (`VOL`), pan (`PAN`), filter mode (`FLAGS0`), any per-sweep enables (`FLAGS1`).
4. Key on by setting the GATE bit in `FLAGS0`.

A simple two-operator FM setup — one carrier, one modulator — at middle A, with a pluck-like envelope:

```asm
    ; Channel 0
    lda #0
    sta $FEFF

    ; Operator 0 = modulator
    lda #%00000000          ; R0: no TRM/VIB/KSR; MUL = 1
    sta $FEC0
    lda #%00011000          ; R1: TL = 24 (moderate modulation depth)
    sta $FEC1
    lda #$E1                ; R2: AR=14 (fast), DR=1 (long decay)
    sta $FEC2
    lda #$07                ; R3: SL=0, RR=7 (medium release)
    sta $FEC3
    lda #0
    sta $FEC4               ; R4: DT=0, SR=0
    sta $FEC5               ; R5: no delay / FIX / WPAR
    lda #%00000010          ; R6: MOD routes to operator 1 (bit 1 of MOD=3-bit dest)
    sta $FEC6
    lda #%00000000          ; R7: OUT=0 (internal only), WAVE=0 (SINE)
    sta $FEC7

    ; Operator 1 = carrier
    lda #%00000001          ; R0: MUL = 1
    sta $FEC8
    lda #0                  ; R1: TL = 0 (loud)
    sta $FEC9
    lda #$F1                ; R2: AR=15, DR=1
    sta $FECA
    lda #$07                ; R3: SL=0, RR=7
    sta $FECB
    lda #0
    sta $FECC
    sta $FECD
    sta $FECE
    lda #%11100000          ; R7: OUT=7 (max), WAVE=0 (SINE)
    sta $FECF

    ; Channel: pitch, volume, gate
    lda #<7256              ; ~A4 at 48 kHz
    sta $FEE0
    lda #>7256
    sta $FEE1
    lda #$40                ; VOL = 64
    sta $FEE2
    lda #0                  ; PAN centre
    sta $FEE3

    lda #%00000001          ; FLAGS0: GATE = 1 (key on)
    sta $FEE4
```

Writing `FLAGS0` with bit 0 cleared releases the note; the envelope will run to its release stage and the channel goes idle. Key-on after release starts a fresh envelope.

## Envelope Shapes

`AR`, `DR`, `SR`, and `RR` are 5-bit rates (`AR` and `DR` are split across `R2` low nibble and `R7` high bits — see Appendix A for the exact packing). `SL` is a 4-bit **level** (the sustain plateau height, not a rate); `TL` is a 7-bit **total level** attenuation split across `R1` low bits and `R6[0]`.

A few practical envelopes:

| Sound      | `AR` | `DR` | `SL` | `SR` | `RR` | `TL` |
| ---------- | ---- | ---- | ---- | ---- | ---- | ---- |
| Percussion | 31   | 14   | 0    | 0    | 15   | 0    |
| Pad        | 10   | 4    | 12   | 2    | 8    | 0    |
| Pluck      | 31   | 4    | 0    | 0    | 7    | 0    |
| Bell       | 31   | 2    | 0    | 4    | 10   | 0    |

These are starting points — FM's timbre depends heavily on per-operator `TL` and routing. Use a short test harness that key-ons on every VBI to iterate fast.

## Filter

Each channel has a **per-channel multimode filter** shared by all of its operators. Cutoff is 16 bits (`CUTOFF_L` / `CUTOFF_H`), resonance is 8 bits (`RESON`, `0` = no feedback, `255` = maximal resonance). Mode is selected by bits in `FLAGS0` (see Appendix A for the exact layout — typically low-pass, high-pass, band-pass, or a compound mode).

```asm
    lda #<4000              ; cutoff value
    sta $FEE6
    lda #>4000
    sta $FEE7
    lda #120                ; RESON: moderate resonance
    sta $FEE9
    lda #%00010001          ; FLAGS0: low-pass mode + GATE
    sta $FEE4
```

Reset the filter state on a new note by setting the "filter reset" bit in `FLAGS1` — useful for percussion sounds that should start cleanly without the prior note's trail.

## Sweeps

Three **per-channel hardware sweeps** — frequency, volume, cutoff — each described by a 16-bit `SPEED`, an 8-bit `AMOUNT` (direction + magnitude), and an 8-bit `BOUND` (travel limit + wrap / bounce behaviour). Enable them with the matching bits in `FLAGS1`.

Canonical uses:

- **Frequency sweep**: vibrato, pitch bends, arpeggios. Fast small amplitude = vibrato; slow large amplitude = glide.
- **Volume sweep**: tremolo, fades, and attack/decay shaping outside the operator envelope.
- **Cutoff sweep**: filter motion that follows the note instead of a global LFO.

Example — a slow upward cutoff sweep that opens the filter after key-on:

```asm
    lda #<200               ; SWCUT_SPEED
    sta $FEF8
    lda #>200
    sta $FEF9
    lda #%01000080          ; SWCUT_AMOUNT: up, moderate
    sta $FEFA
    lda #$E0                ; SWCUT_BOUND
    sta $FEFB

    lda #%01000000          ; FLAGS1: cutoff-sweep enable
    sta $FEE5
```

(Exact `AMOUNT` / `BOUND` / `FLAGS1` bit positions are in Appendix A.)

## PCM Playback

Any channel can switch from FM synthesis to **PCM playback** by setting the PCM-enable bit in `FLAGS0` (bit 3). The channel's three 16-bit pointers then drive playback:

- `PCM_POS` — the current play position (initialized to where you want playback to start).
- `PCM_END` — the end boundary. When `PCM_POS` reaches this, the channel either stops or loops.
- `PCM_RST` — the loop restart position (used when the `PCM_LOOP` bit is set in `FLAGS1`).

PCM sample data lives in the audio chip's internal 64 KB sample RAM, addressable by these 16-bit pointers. Loading a sample into that memory is done through a separate firmware path (asset loader); from the CPU side, a program treats the sample region as opaque once loaded.

A looping one-shot:

```asm
    lda #<sample_start
    sta $FEEA               ; PCM_POS_L
    lda #>sample_start
    sta $FEEB               ; PCM_POS_H
    lda #<sample_end
    sta $FEEC               ; PCM_END_L
    lda #>sample_end
    sta $FEED               ; PCM_END_H
    lda #<sample_loop
    sta $FEEE               ; PCM_RST_L
    lda #>sample_loop
    sta $FEEF               ; PCM_RST_H

    lda #%00000100          ; FLAGS1: PCM_LOOP
    sta $FEE5
    lda #%00001001          ; FLAGS0: PCM + GATE
    sta $FEE4
```

Pitch of the playback is driven by the channel's `FREQ` register as with FM channels, so the same sample can be re-pitched across the keyboard for sampler-style instruments.

### PCM as a wavetable operator

A subtler use of the sample region: any operator can pick `WAVE = 7` (SAMPLE), which treats a 1024-sample slice of the sample region as a wavetable, with `PCM_RST` pointing at the slice base. This lets a channel mix a sampled voice as the modulator or carrier in an otherwise-FM patch — the defining flexibility of the SGU-1 over pure-FM chips.

## System Buzzer

The buzzer is a separate, simpler subsystem at `$FFA8–$FFAB` (see [Chapter 6](../1/6_io.md)), intended for the sort of blip-and-beep the OS or an application wants without spinning up a full SGU voice.

```asm
    ; ~1 kHz beep at 50% duty for a short time
    lda #$FF                ; FREQ_L
    sta $FFA8
    lda #$8C                ; FREQ_H (together encodes ~1 kHz — see Chapter 6)
    sta $FFA9
    lda #$80                ; DUTY = 50%
    sta $FFAA
    ; ... wait ...
    lda #$00
    sta $FFAA               ; DUTY = 0 silences
```

The frequency is encoded logarithmically, so a byte-table lookup for common pitches (note names to FREQ values) is a practical way to "play notes" on the buzzer. For any real music, use SGU-1 instead.

## Summary

Programming the SGU-1 is register-driven and mostly stateless: select the channel, write its registers, key it on. The channel-switched window keeps the per-channel programming compact; hardware envelopes, sweeps, and PCM mode keep the CPU burden low even for rich patches. The tracker plus the shipped player routines handle the song-engine layer; the buzzer covers the system-beep use case without touching the synth. Full register / bit layouts are in [Appendix A](../A/A_memory_map.md).
