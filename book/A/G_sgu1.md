# Appendix G: The SGU-1 Sound Generator

[Chapter 5](../1/5_audio.md) introduces the SGU-1 as a part of the machine, [Chapter 12](../2/12_sound.md) shows the register sequences that drive it, and [Appendix A](A_memory_map.md) lists every register and bit. This appendix is the reference the other three lean on: **why the chip exists, what it is made of, and what its parameters actually do** — the numbers behind the fields, the order the signal travels in, and the ancestor each feature was borrowed from.

## Why the X65 Has Its Own Sound Chip

Every classic machine in [Appendix D](D_systems_comparison.md) is remembered partly through its sound chip. Rebuilding that experience today runs into a supply problem: the interesting parts — the SID, POKEY, the OPL family, Paula — have not been manufactured for decades, and what remains in circulation is pulled from dead hardware, priced accordingly, and often counterfeit. Designing around a chip that cannot be bought is not a design.

The X65 started with a chip off the shelf. The **Yamaha YMF825** (Sound Designer 1 — SD-1) was the machine's synthesizer from late 2024, first over SPI and memory-mapped into the 65816's own address space so that CPU code poked its registers directly. Around it grew the rest of an audio section: two PWM channels for simple tones and sampled effects, and a ROHM analog mixer — a BD3461FS on the breadboard, a BD34602FS-M in the specified machine — folding synthesizer, PWM, line-in and an expansion pair down to stereo line-out. Even that mixer was a scavenge: digitally controlled audio mixers stopped being manufactured over a decade ago, so the X65 repurposed an automotive chip for the job. In 2025 the PWM sample channels gave way to an SGTL5000 CODEC, which did the same work better and brought a line input with it.

It worked, and it did not last. The YMF825 is out of production and what supply remains is unreliable — the same fate that took the SID and POKEY, arriving recently enough to catch this project mid-build. Late in 2025, after a long discussion with the community, the X65 dropped SD-1 rather than ship a machine whose defining voice depended on a part nobody makes. The redesign kept a CODEC in the audio path and dropped the analog mixer along with SD-1. The synthesizer had to come from somewhere else.

So the X65 builds its own. The **SGU-1 (Sound Generator Unit 1)** takes its operator model from **tildearrow's Sound Unit**, the fantasy synthesis chip built into the Furnace tracker, and wraps it in a memory-mapped register interface sized for the X65. Around that core it deliberately cherry-picks from the machines it is meant to stand beside: FM operators and routing from OPL3 and ESFM, envelopes and detune from the OPN/OPM line, the resonant filter and pulse-width behaviour of the SID, POKEY's short-LFSR noise, and Paula's sample voices.

| Borrowed from | What SGU-1 takes from it |
| --- | --- |
| OPL3 / ESFM | Per-operator routing instead of fixed algorithms, OPL-style frequency multipliers, key-scale level, the half/absolute waveform variants |
| OPN / OPM | The five-stage AR/DR/SL/SR/RR envelope, sign-magnitude detune applied before the multiplier, an LFO counter with a noise LFSR |
| OPZ | Per-operator fixed-frequency mode |
| SID | The resonant multimode filter, level-driven gate keying, signed pulse width, and a phase increment clocked as if from a 1 MHz oscillator |
| POKEY | Periodic noise from a short LFSR with selectable taps |
| Paula | Signed 8-bit PCM voices with loop points, available on any channel |

Because none of this is silicon — the entire chip is software running on a microcontroller — it could be built at all, and it could keep growing after the first prototype made a sound.

### How it got here

| Date | Milestone |
| --- | --- |
| September 2024 | The Yamaha YMF825 (**SD-1**) becomes the machine's FM synthesizer, mixed with two PWM channels through a repurposed automotive mixer chip |
| May 2025 | An SGTL5000 CODEC takes over sample playback from the PWM channels and adds a line input |
| November 2025 | SD-1 is memory-mapped into the 65816's address space and plays its first test tune under CPU control |
| December 2025 | SD-1 is dropped — out of production, supply unreliable — in favour of a custom sound processor on a microcontroller of its own, driving a modern CODEC/DSP |
| December 2025 | SGU-1 announced; a proof-of-concept SID-to-SGU register translation plays Rob Hubbard's *Monty on the Run* in the web emulator |
| January 2026 | First hardware tech demo — a microcontroller and a CODEC on a breadboard, rendering a six-channel SID translation as 48 kHz stereo I²S |
| February 2026 | Feature-complete: nine channels, all eight waveforms, ESFM routing, resonant filters, sweeps, PCM |
| April 2026 | The module's CODEC settles on a TI AIC3254, integrated on the SGU-1 itself |
| May 2026 | First test batch of SGU-1 modules arrives |
| June 2026 | The synthesis core is imported into **SGU Tracker**, the chip's native tracker |

:::{note}
SGU-1 is an engineering prototype. Its electrical limits, pin assignments and production specifications are not final, and revision 1 data must not be used for production designs. The programming interface described here — the register window and its semantics — is the stable part.
:::

## What the Chip Is

Physically, an SGU-1 is a small castellated module carrying a microcontroller — an **RP2354**, the RP2350 variant with flash stacked in the package — and an audio CODEC. The microcontroller runs the synthesis engine and emits a stereo 48 kHz I²S stream; the CODEC turns that into analog stereo line-out and is itself configured over I²C for gain, mute and routing.

The CODEC chosen is a **TI AIC3254**, a converter with its own on-chip DSP — the "modern CODEC+DSP" the project set out to pair with a custom synthesizer. The breadboard prototype used an SGTL5000 for simplicity, reusing the rev 1 DEV-board firmware work.

The X65 DEV-board hosts a complete SGU-1 module — the same castellated part described above — bridged from the SOUTH chip over SPI and presented to the CPU as the `SPU` device on the PIX bus. Writes to `$FEC0–$FEFF` are picked up by NORTH, forwarded to SOUTH, and relayed over SPI; see [Chapter 5](../1/5_audio.md) for the full path.

Everything programmable in the synthesis engine is **per channel or per operator**. The clocks are chip-level — the 48 kHz sample rate, the 16 kHz envelope tick, the LFO counter — but every tap on them belongs to a channel or an operator, so there is no global tuning, no global envelope rate, no global LFO speed. The chip-wide controls that do exist — master volume and routing — live in the service bank, and they address the CODEC and its DSP downstream of the waveform generator rather than the synthesis engine itself.

The synthesis engine has a hard real-time budget. At 336 MHz and 48 kHz that is **7000 cycles per core per sample**, so the nine channels are split across both cores of the RP2354 — one core sets up the shared per-sample state, renders five channels and merges; the other renders the remaining four in an inter-core interrupt. Overrunning the sample deadline is treated as a fault, not as a dropout: the firmware reports it, latches an error colour on the status LED and stops. This is the reason features are admitted to the chip by measurement rather than by taste.

## The Signal Path

Everything below happens once per sample, per channel, in this order:

```text
          feedback
          .-----.
          v     |
         OP0 ---+---> OP1 ---> OP2 ---> OP3     phase modulation, scaled
          |           |         |        |      by each operator's MOD level
          '-----------+---------+--------'
                      |  scaled by each operator's OUT level
                      v
                 channel mix
                      |
                      v
        ring modulate by channel N+1     (FLAGS0 bit 4)
                      |
                      v
                 channel VOL             (signed: negative negates the sample)
                      |
                      v
        state-variable filter, taps summed   (FLAGS0 bits 5-7)
                      |
                      v
                 PAN into L / R
                      |
   nine channels -----+---> sum ---> DC-removal high-pass ---> saturating limiter ---> I2S
                                                                                        |
   ------------------------------------------------------------------------------------'
   |
   '-> CODEC / DSP / mixer:  master volume, routing  ---> analog line out
```

Two details in that order are easy to get wrong and easy to hear. **The channel volume is applied before the filter**, so it drives the filter as well as scaling the output — a resonant patch changes character with volume. And **the filter is bypassed entirely when no output tap is selected**: with FLAGS0 bits 5–7 all clear, the cutoff and resonance registers do nothing at all.

## The FM Core

### Operators and Routing

Each channel owns four operators. Like ESFM — and unlike OPL or OPN — there is **no algorithm selector**. Instead every operator carries two 3-bit level fields, and the pair of them defines the topology:

- **`OUT`** (`R7[7:5]`) — how much this operator contributes directly to the channel mix. `0` disconnects it; `1`–`7` are 6 dB steps, `7` being unity.
- **`MOD`** (`R6[3:1]`) — how strongly the **previous** operator phase-modulates this one, in the same 6 dB steps. On operator 0 there is no previous operator, so `MOD` becomes the **feedback** level, exactly as in ESFM.

An operator can carry both fields at once, so it is free to be a modulator and a carrier simultaneously — a structure the Yamaha algorithm tables cannot express. The classic algorithms remain available as particular settings:

| Resembles | `OUT0` | `MOD1` | `OUT1` | `MOD2` | `OUT2` | `MOD3` | `OUT3` |
| --- | :-: | :-: | :-: | :-: | :-: | :-: | :-: |
| OPN algorithm 0 (serial chain) | 0 | 7 | 0 | 7 | 0 | 7 | 7 |
| OPN algorithm 4 | 0 | 7 | 7 | 0 | 0 | 7 | 7 |
| OPN algorithm 6 | 0 | 7 | 7 | 0 | 7 | 0 | 7 |
| OPN algorithm 7 (fully parallel) | 7 | 0 | 7 | 0 | 7 | 0 | 7 |
| OPL3 algorithm 1 | 7 | 0 | 0 | 7 | 0 | 7 | 7 |
| OPL3 algorithm 3 | 7 | 0 | 0 | 7 | 7 | 0 | 7 |
| OPL3 algorithm 1, variant | 0 | 7 | 0 | 7 | 7 | 0 | 7 |

`MOD0` is omitted from the table because it is feedback rather than routing.

Two more per-operator couplings run alongside the modulation chain, both referring to the previous operator (operator 3 for operator 0):

- **`SYNC`** (`R6[5]`) resets this operator's phase whenever the previous operator's phase wraps — hard sync, and the fastest route to aggressive, harmonically dense timbres.
- **`RING`** (`R6[4]`) is a one-bit ring modulator: this operator's output is negated whenever the previous operator's output is negative. Cheap, and it produces the metallic sum-and-difference tones the name promises.

:::{note}
This book numbers operators **0–3**, matching the register offsets (`$00`, `$08`, `$10`, `$18` inside the channel window). The canonical memory-map sheet and the tracker interface number the same four operators **1–4**. Only the labels differ.
:::

### Pitch

A channel has **one** base pitch, the 16-bit `FREQ` register, and each operator derives its own frequency from it. `FREQ` is a phase increment with SID semantics — as if clocked at 1 MHz into a 24-bit accumulator:

```text
Hz   = FREQ * 1000000 / 2^24      (about FREQ * 0.0596)
FREQ = Hz * 2^24 / 1000000        (about Hz * 16.777)
```

Concert A (440 Hz) is `FREQ = 7382` (`$1CD6`). A note table is the practical way to do this on the CPU; the arithmetic is a 24-bit multiply the 65816 would rather not do per note.

From that base, each operator applies:

- **`MUL`** (`R0[3:0]`) — an OPL-style multiplier: `0` means half frequency, and `1`–`15` map to `1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 10, 12, 12, 15, 15`. (The duplicated entries are OPL's, kept for fidelity when porting patches.)
- **`DT`** (`R4[7:5]`) — Yamaha sign-magnitude detune, exactly as OPN and OPM spell it: bit 2 is the sign, bits 1–0 the magnitude. **`0` and `4` both mean no detune**; `1`–`3` detune up and `5`–`7` down. The shift is keycode-dependent, from under a cent at the bottom of the range to roughly 9 cents at the top, and it is applied *before* the multiplier, so the effect scales with `MUL`.

### Fixed Frequency

Setting **`FIX`** (`R5[4]`) takes an operator off the channel pitch entirely, in the OPZ tradition. `MUL` and `DT` are then reinterpreted as an absolute frequency:

```text
freq16 = base[MUL] << DT
base   = { 8, 24, 41, 57, 74, 90, 107, 123, 140, 156, 173, 189, 206, 222, 239, 255 }
```

which spans about 0.48 Hz to 1945 Hz. Detune and vibrato do not apply in this mode. Fixed operators are what give percussion its non-pitched partials: a drum whose body follows the note but whose click does not.

### Waveforms

Every operator picks its own waveform, and every waveform reads the 4-bit **`WPAR`** field (`R5[3:0]`) for shaping. This is where a channel stops sounding like an FM chip if you want it to.

| `WAVE` | Waveform | `WPAR` meaning |
| :-: | --- | --- |
| 0 | Sine | Shape variants (below) |
| 1 | Triangle | Shape variants (below) |
| 2 | Sawtooth | Shape variants (below) |
| 3 | Pulse | `0` uses the channel `DUTY` register; `1`–`15` fix this operator's width at *x*/16 |
| 4 | Noise | 32-bit LFSR white noise, clocked from the channel frequency |
| 5 | Periodic noise | 6-bit LFSR; `WPAR[1:0]` picks the taps: `0` = 3,4 · `1` = 2,3 · `2` = 0,2,3 · `3` = 0,2,3,5 |
| 6 | *reserved* | Outputs silence |
| 7 | Sample | 1024 bytes of PCM memory used as a wavetable (see below) |

For sine, triangle and sawtooth, `WPAR` bit 3 chooses between two families:

- **Bit 3 clear** — OPL-style variants, split at the channel's `DUTY` point: `1` silences the wave before the split, `2` silences it after, `3` negates before, `4` negates after. These are OPL3's half-sine and absolute-sine shapes, generalised to all three base waves and to a movable split point.
- **Bit 3 set** — quantization: bits 0–2 select how many low bits of the wavetable index are zeroed (`bits + 1`), producing stepped, lo-fi versions of the base wave.

The two noise waveforms cover different territory. `NOISE` is a long LFSR, the hiss you want under a snare or an explosion. `PERIODIC_NOISE` is a 31- or 63-state LFSR whose tap configuration is per-operator — the tonal, metallic buzz that gives POKEY and the SID their distinctive percussion, and four flavours of it inside one channel if you want them.

### Envelopes

Each operator has its own five-stage envelope: attack to peak, decay to the sustain level, a second decay while the key is held, then release. The second decay rate (`SR`, OPN's D2R) is what OPL lacks, and it is the difference between a plucked string that keeps fading and one that sits on a plateau.

| Field | Bits | Location | Meaning |
| --- | :-: | --- | --- |
| `AR` | 5 | `R2[7:4]` + `R7[4]` | Attack rate; higher is faster |
| `DR` | 5 | `R2[3:0]` + `R7[3]` | First decay rate, towards `SL` |
| `SL` | 4 | `R3[7:4]` | Sustain level as OPN-polarity **attenuation**, about 3 dB per step: `0` never drops, `15` decays to silence |
| `SR` | 5 | `R4[4:0]` | Second decay rate while the key is held; `0` freezes the level |
| `RR` | 4 | `R3[3:0]` | Release rate after key-up |
| `TL` | 7 | `R1[5:0]` + `R6[0]` | Total level: output attenuation in 0.75 dB steps, `0` loudest |
| `KSL` | 2 | `R1[7:6]` | Attenuation that increases with pitch, following the OPL3 law |
| `KSR` | 2 | `R0[5:4]` | Envelope rate scaling with pitch |
| `DELAY` | 3 | `R5[7:5]` | Key-on delay: `0` none, otherwise 2^(`DELAY`+8) samples — 10.7 ms to 683 ms |

Rates are 5-bit where OPL's are 4-bit, `TL` is 7-bit where OPL's is 6-bit, and `KSR` is 2-bit where OPL's is one. The envelope generator itself runs at 16 kHz, one third of the audio rate, which makes SGU-1 envelopes about 10 % slower than an ESFM reference at the same rate numbers — worth knowing when porting patches by hand.

`DELAY` deserves a mention on its own: it staggers key-on per operator, so a multi-operator attack can be choreographed — a click, then a body, then a swell — with no CPU involvement beyond the single key-on write.

### GATE and TRIG

Keying uses two bits with distinct jobs, and knowing which does what is the difference between a tie and a retrigger.

`FLAGS0` bit 0 (**`GATE`**) is the key **level** — high while the key is held, low to let it go. What a high `GATE` does depends on the state the envelope is already in:

| `GATE` | Envelope state | Result |
| --- | --- | --- |
| High | Release (or idle) | **Enter attack**, rising from whatever attenuation the envelope currently sits at. An idle voice is fully attenuated and so attacks from silence; one still ringing out attacks from the level its release had reached |
| High | Attack / decay / sustain | The envelope **runs on** — no restart. Hold the key, write a new `FREQ`, and the note slurs |
| Low | any | Enter release |

Release is the state that means "key up", so a `GATE` cycle — low, then high again — re-attacks from the level the release had got to. That is the SID's model exactly, and it is a **soft** retrigger: the envelope resumes rather than starting over.

Writing `GATE` to a voice in attack, decay or sustain leaves its envelope untouched. That is what makes a tie or a legato slur expressible: `GATE` alone cannot restart a note that is still sounding.

**`TRIG`** (bit 1) covers everything `GATE` cannot. It is a one-shot, self-clearing request that scrubs every operator's envelope to full attenuation — silence, in release — and arms the per-operator `DELAY` window, so the note attacks from nothing regardless of what it was doing when the write landed. It is consumed at the next processed sample, so a read-back shows it set for less than one sample. It is the **only** way to restart a note that is already sounding, and the only way to make a note attack from true silence rather than from a decaying tail.

Read together, the two bits spell out the four note events a tracker or driver needs, each of them a single write:

| `GATE` | `TRIG` | Note event |
| :-: | :-: | --- |
| 1 | 0 | **Key down** — a sounding envelope runs on (a tie or legato slur); a released or idle one attacks from its current level |
| 1 | 1 | **Note-on** — restart the envelope from silence, out of any state |
| 0 | 0 | **Note-off** — release ramp |
| 0 | 1 | **Note-cut** — instant silence |

This is worth dwelling on, because it removes a timing hazard that every edge-triggered chip imposes. With an edge-triggered gate, retriggering a sounding voice means dropping the gate, *making sure the chip renders at least one sample with it low*, and raising it again — at 48 kHz that window is 20.8 µs, which even a fast CPU can miss entirely. A level gate plus a one-shot retrigger bit has no such window: note-on is `ORA #$03`, legato is `ORA #$01`, and neither depends on how fast the CPU is running.

### Playing Under a Held Gate

Every register is read by the synthesis engine as it renders each sample, so a write lands on the next sample and takes effect mid-note, whatever the envelope happens to be doing. Editing a sounding voice is the intended technique, and for a whole family of musical gestures it is the *only* one available.

The reason is the envelope. While `GATE` stays up, nothing but `TRIG` can restart it and nothing but dropping `GATE` can release it. So any effect that has to preserve the envelope's position — a tied note, a slide, a swell, a filter opening across a phrase — is produced by writing *other* registers with `GATE` held and `TRIG` clear. Nothing else in the register file disturbs the envelope.

| Gesture | Written under a held gate |
| --- | --- |
| **Legato, tie** | `FREQ`. The new pitch takes over without re-attacking: a key-down with `TRIG` clear changes nothing about a voice that is already sounding |
| **Slide, portamento** | `FREQ`, stepped per tick by the driver — or armed once as a frequency sweep and left to the chip |
| **Vibrato** | `FREQ` per tick, or the LFO's PM with `VIB` set on the operators that should follow it |
| **Swell, tremolo, fade** | `VOL`, or the volume sweep, which runs regardless of key state |
| **Filter motion** | `CUTOFF` and `RESON`, or the cutoff sweep |
| **PWM** | `DUTY`, whose signed wrap is what lets the sweep run continuously |
| **Timbre morphing** | Operator `TL`, `MOD` and `OUT` — the FM equivalent of opening a filter |

Two cautions come with the freedom.

Hardware sweeps **write back into the registers they animate**: an armed frequency sweep updates `FREQ`, a volume sweep updates `VOL`, a cutoff sweep updates `CUTOFF`. Driving one of those from the CPU while its sweep is enabled means two writers are fighting over the same byte, and the result is neither gesture. Pick one per parameter, per note.

And `FLAGS0` carries `TRIG` alongside the filter taps and the PCM bit, so it is the one register a mid-note edit has to be careful with. Because `TRIG` self-clears within a sample, reading the register back will not normally show it — but any shadow byte or baked instrument image that has the bit set will re-fire the retrigger on the next write and reset the envelope that was being preserved. This is why SGU-Tracker instruments are stored key-less: exporters mask `GATE` and `TRIG` out of a baked `FLAGS0`, and drivers keep their own shadow so an unrelated write to the filter bits cannot retrigger a note. On the 65816 the two note events are then one instruction apart — `ORA #$01` for a key-down, `ORA #$03` for a note-on.

## The Subtractive Back End

Once the four operators are summed, the channel becomes a small subtractive synthesizer.

### Ring Modulation, Volume and Pan

`FLAGS0` bit 4 multiplies the channel's FM output by the raw output of the **next** channel (channel 8 wraps to channel 0) — full amplitude ring modulation between voices, distinct from the one-bit per-operator `RING`.

`VOL` and `PAN` are both **signed** bytes. The channel's samples are multiplied by `VOL`, so a negative volume **negates the sample values** — every sample keeps its magnitude and flips its sign. For a waveform that happens to be symmetric about zero this is indistinguishable from a half-cycle phase shift, but that equivalence is a property of the waveform, not of the register: an asymmetric one is genuinely turned upside down, not moved in time. It matters when two channels are summed or ring-modulated. A negative pan places the voice left, positive right.

### The Filter

Each channel carries a **state-variable filter** with a 16-bit cutoff and 8-bit resonance — the same architecture that gave the SID its voice. `FLAGS0` bits 5 (low-pass), 6 (high-pass) and 7 (band-pass) select which outputs are summed, so the compound modes fall out naturally: low + high is a notch, low + band a gentler roll-off, and so on.

Resonance does double duty: it reduces the filter's internal damping *and* increases the drive into it, so high resonance settings get louder as well as sharper. And, as noted above, **selecting no taps bypasses the filter entirely** — the dry channel, not a wide-open filter.

`FLAGS1` bit 1 requests a one-shot reset of the filter state. Percussion that should start cleanly, rather than on top of the previous note's ringing, wants it on every trigger.

### Pulse Width

The channel `DUTY` register is a **signed** byte. The magnitude is the length of the low run out of the 128-step period, and the sign places that run at the start of the period (positive, `____|~~~~`) or at the end (negative, `~~~~|____`). Negating the duty therefore mirrors the wave in time; `0` is all high, `-128` all low.

The consequence is that a duty sweep is continuous through **both** wraps, `127 → -128` and `-1 → 0`. The classic wrapping PWM sweep — the one that gives 8-bit basses their motion — plays without a snap at either rail. The same split point drives the half and absolute variants of sine, triangle and sawtooth, so `DUTY` shapes those waveforms too.

### Sweeps

Three per-channel sweep units retune a parameter without CPU involvement. Each has a 16-bit `SPEED` (samples between steps, `0` disables), an 8-bit `AMT` carrying step size, direction and mode, and an 8-bit `BOUND`. All three are enabled from `FLAGS1`.

| Sweep | Enable | Step law | `BOUND` |
| --- | --- | --- | --- |
| Frequency | `FLAGS1[4]` | Exponential — up multiplies by (128+step)/128, down by (255−step)/256. Step is 7 bits, direction is bit 7 | Coarse target: saturates exactly at `BOUND << 8` |
| Volume | `FLAGS1[5]` | Linear signed addition of a 5-bit step. Direction bit 5, loop bit 6, bounce bit 7 | Signed target, which also divides the range into the two segments a looping sweep runs between |
| Cutoff | `FLAGS1[6]` | Asymmetric — upward is linear, downward multiplicative | Coarse target: saturates exactly at `BOUND << 8` |

The exponential frequency law is what makes it a portamento rather than a chirp: equal steps are equal musical intervals. The volume sweep is the only one with run modes — one-shot, repeat, and ping-pong — which is what turns it into a tremolo or a repeating fade rather than a single gesture. Volume sweeps run regardless of key state, so a fade continues through a note-off.

### The Phase-Reset Timer

`RESTIMER` is a 16-bit period; with `FLAGS1` bit 3 (`TIMER_SYNC`) set, the channel's phase is reset every that many samples. Independently, `FLAGS1` bit 0 requests a single immediate phase reset.

Periodic phase reset at an audio-rate period is a sound generator in itself — it imposes a second periodicity on the waveform, the effect that makes hard-synced leads and formant-like timbres possible without spending an operator on them.

### The LFO

The LFO is configured per channel. `LFOW` picks a shape for amplitude modulation and, independently, one for pitch modulation:

| Value | AM shape (`LFOW[1:0]`) | PM shape (`LFOW[3:2]`) |
| :-: | --- | --- |
| 0 | Saw | Saw |
| 1 | Square | Square |
| 2 | Triangle | Triangle |
| 3 | Noise | Noise |

Operators opt in individually — `TRM` (`R0[7]`) for tremolo, `VIB` (`R0[6]`) for vibrato — and pick one of two depths with `TRMD` and `VIBD` (`R6[7:6]`). Shallow tremolo is a quarter of the deep setting; deep vibrato peaks at roughly ±13.5 cents, shallow at about half that.

What a channel chooses is the shape and the depth. The rate is fixed — the pitch-modulation phase cycles at about **5.86 Hz** and the amplitude-modulation phase at about **2.93 Hz** — and both are read from one free-running counter, so LFOs on different channels stay in step with each other.

The noise shapes are the least conventional of the four: noise-shaped PM is a fast, unstable detune that reads as an aggressive edge on a lead, and noise-shaped AM is a texture no vintage FM chip offers.

## PCM

Any channel can leave FM behind and play back samples instead, and any operator can use a sample as its waveform. Both read the same **64 KB of PCM memory**, which lives inside the audio chip and is *not* visible in the 65816's address space.

### Channel PCM Mode

Setting `FLAGS0` bit 3 switches the channel to sample playback, driven by three 16-bit pointers: `PCMPOS` (current position), `PCMBND` (end boundary), and `PCMRST` (loop restart). With `FLAGS1` bit 2 (`PCM_LOOP`) set, reaching the boundary jumps to the restart point instead of stopping. Sample data is signed 8-bit.

In PCM mode `FREQ` is a playback rate rather than a pitch:

```text
rate_Hz = 48000 * FREQ / 32768
```

so `FREQ = 32768` plays back at the native 48 kHz, and half that plays an octave down. The rate is clamped at 1:1 — a sample cannot be pushed above its own rate — and playback wraps within the active 64 KB bank.

### Samples as Wavetables

An operator with `WAVE = 7` reads a **1024-byte slice** of PCM memory as its wavetable, based at the channel's `PCMRST` pointer, with the operator's phase indexing into it and wrapping naturally at the end. That operator behaves in every other respect like an FM operator: it has its own envelope, it can modulate and be modulated, it can be detuned and fixed-frequency.

This is the flexibility that distinguishes SGU-1 from a pure FM chip. A sampled attack transient can be the modulator for a synthesized body; a single-cycle wavetable can be the carrier that four operators shape. And it costs nothing beyond the sample memory the channel was already able to reach.

### Getting Samples Into the Chip

Because PCM memory is not CPU-addressable, samples are pushed through a port in the SGU-1 **service bank**, selected by writing `$FF` to the channel selector. The service bank occupies the same 64-byte window, and reaches past the synthesis engine to the sample memory and to the CODEC/DSP that follows it:

| Address | Register | Notes |
| --- | --- | --- |
| `$FEDC` | Sample offset, low byte | Auto-increments on every access to the data port |
| `$FEDD` | Sample offset, high byte | Wraps `$FFFF → $0000` |
| `$FEDE` | Sample bank | Selects a 64 KB bank; only bank `0` is backed today |
| `$FEDF` | Sample data port | Write stores a byte at the current offset, read returns it; both bump the offset |
| `$FEE0` | Master volume | Gates the entire mix. **Resets to `0` — the chip comes up muted** |
| `$FEF0/$FEF1` | Master mix left, lo/hi | The exact 16-bit I²S output word (read-only, live) |
| `$FEF2/$FEF3` | Master mix right, lo/hi | Same, right channel |
| `$FEF4/$FEF5` | Sample counter, lo/hi | Low word of the free-running 48 kHz sample clock — a timestamp for readers |
| `$FEF6` | LFO AM phase | Global LFO amplitude-modulation phase, `0`–`255` (read-only, live) |
| `$FEF7` | LFO PM phase | Global LFO pitch-modulation phase, `0`–`255` (read-only, live) |

Uploading is therefore a select, three setup writes, and a tight blit:

```asm
    lda #$FF
    sta $FEFF               ; select the service bank
    lda #$00
    sta $FEDE               ; sample bank 0
    sta $FEDC               ; offset $0000, low
    sta $FEDD               ; offset $0000, high

    ldx #$00
upload:
    lda sample_data,x
    sta $FEDF               ; data port; the offset auto-increments
    inx
    bne upload
```

Note that this transfer touches no channel state at all: service-bank traffic and channel traffic are independent, so a sample can be uploaded while other voices are sounding — only the selector has to be restored afterwards.

:::{warning}
**SGU-1 is silent until software unmutes it.** The master volume gates the whole mix and resets to zero, deliberately: a reset must never blast the user with whatever the register file powered up holding. Wipe the channels first, then raise `$FEE0` — see [Bringing the Chip Up](#bringing-the-chip-up) below. Under OS/816 that is the system's job; a bare-metal program must do it itself, or it will hear nothing and blame its patch.
:::

:::{note}
The selector accepts `$00`–`$08` for the nine channels and `$FF` for the service bank; `$09`–`$FE` are reserved. Portable software should write only those defined values — some builds of the audio firmware still fold an out-of-range selector onto a channel, where a stray write would corrupt a sounding voice.
:::

### Diagnostic Readback

`FLAGS1` bit 7 (`DIAG`) turns the channel's own register window into a meter. While it is
set on a channel, a handful of that channel's offsets become **dual-function**: writes still
land in the register file as always, but reads return live chip state instead of the stored
byte. Everything not listed — and every offset while `DIAG` is clear — reads back normally.
All values are pre-`VOL`, pre-pan.

| Window offset | Diagnostic read |
| --- | --- |
| op *n* base+0 | Operator envelope attenuation in 0.375 dB steps: `0` = full level, `255` = silent |
| op *n* base+1 | Envelope state in bits 1:0 (attack/decay/sustain/release); bit 2 set while the `TRIG`-armed key-on delay window is running |
| op *n* base+2/+3 | The operator's current sample, signed 16-bit, lo/hi |
| `$20`/`$21` (`FREQ` slots) | The raw channel mix, signed 16-bit, lo/hi — before volume, filter and pan; live for PCM voices too |
| `$22` (`VOL` slot) | The channel envelope level, `0`–`255` linear — the OUT-weighted sum of the operator envelopes; `0` on a PCM voice |

This is how a player draws VU meters that agree with the chip instead of re-modelling its
envelopes — `TRIG` retriggers, invisible in a plain readback, show up here for free. Two
conventions keep it safe alongside a running driver: drivers never set bit 7 (they compose
`FLAGS1` from their own shadow), so any ordinary `FLAGS1` write drops the channel back to
normal readback; and a reader that finds bit 7 cleared mid-pass — or the channel selector
moved — throws the pass away and retries. The two bytes of a 16-bit sample are separate
bus reads while the chip renders at 48 kHz, so a pair can straddle a sample boundary —
jitter a scope view can live with, not corruption. Diagnostic readback exists from chip
version 1.1; the version registers in the service bank identify it.

## The Output Stage

The nine panned channels are summed, and the mix passes through a one-pole DC-removal high-pass — a corner near 10 Hz, low enough to be inaudible and high enough to keep asymmetric waveforms from eating headroom. The result is saturated into the 16-bit I²S word, and that is where the microcontroller's work ends. It emits that stream at **full scale**, applying no master attenuation of its own; the module's second chip — the CODEC and its DSP/mixer — is what applies master volume to it.

That final saturation clips rather than wraps: a mix past full scale is held at the rail. The engine latches a sticky flag when it happens, which is how the tracker's clip indicator knows to light up.

The order matters when a mix distorts. Master volume sits **downstream of the limiter, in the other chip**, so turning it down makes an already-clipped mix quieter without making it clean — the saturation happened a chip earlier. Headroom is managed with per-channel `VOL`; the master sets how loudly the finished mix is played. Nine channels at full volume will reach the rail easily.

## Bringing the Chip Up

A cold machine hands software a chip that is already zeroed and already muted. Neither fact can be relied on by a program that starts warm — after a monitor, an OS shell, or another program that was making sound — so bring-up is a sequence, not an assumption:

1. **Reset**, by wiping every channel's registers to zero. This is exactly what a hardware reset does to the register file, and it is the only way to be sure no gate is held, no sweep is armed and no PCM voice is looping.
2. **Unmute**, by raising the service bank's master volume.
3. **Select** a channel and **configure** it — operators, then channel-wide controls.
4. **Set the channel's volume.** `VOL` is zero after the reset, and a channel at zero is silent however carefully its operators are programmed. This is the single most common reason a correct-looking patch makes no sound.
5. **Key on**, with `GATE` and `TRIG` together.

Steps 3 to 5 repeat per channel; steps 1 and 2 happen once.

```asm
    ; 8-bit A/X/Y (SEP #$30), data bank $00
    ldy #8                  ; reset: channels 8 down to 0
reset_ch:
    sty $FEFF               ; select the channel
    ldx #$3E                ; window offsets $3E..$00
    lda #$00
reset_reg:
    sta $FEC0,x             ; $FEFF, the selector itself, is left alone
    dex
    bpl reset_reg
    dey
    bpl reset_ch

    lda #$FF
    sta $FEFF               ; select the service bank
    lda #$FF
    sta $FEE0               ; master volume -- unmute
```

There is no chip-level reset register: the register window is the whole CPU-visible interface, so a software reset means writing the zeroes yourself. Two details in that loop are easy to get wrong. It leaves the selector at `$FEFF` alone, which is the one byte of the window that is not a channel register; and `sta $FEC0,x` is an absolute indexed write, so it resolves against the **data bank** — with `DBR` set to anything but `$00`, the loop cheerfully wipes 512 bytes of somebody else's memory instead. The index registers must be 8-bit as well, or `dex`/`bpl` counts down from `$003E` through sixteen thousand more addresses.

[Chapter 12](../2/12_sound.md) carries this through into a complete voice.

## Standing In For Other Chips

Much of the SGU-1's public demonstration has consisted of it playing music written for other machines — SID tunes, POKEY tunes, OPL tunes — through a register translation rather than a re-arrangement. That is a deliberate test, and it works for structural reasons:

- **SID** maps almost directly. Its gate is a level, like SGU-1's; its filter is the same state-variable topology; its pulse width, ring modulation and hard sync all have counterparts. A SID voice becomes a single-operator SGU-1 channel with the filter engaged, and the translation needs no timing tricks.
- **POKEY** relies on short-LFSR "distortion" settings, which is what `PERIODIC_NOISE` and its four tap configurations reproduce.
- **OPL and ESFM** patches carry over field by field: the multipliers, key scaling, and half/absolute waveform variants are OPL's, and the missing algorithm selector is expressible as `OUT`/`MOD` pairs from the table earlier in this appendix.

What does not carry over automatically is anything that depended on a chip's *defects* — a specific filter nonlinearity, an ADSR bug, the exact noise spectrum of one LFSR. Translations get the notes and the shape of the sound; they do not get the silicon.

Recordings of the chip standing in for its ancestors are collected in the [SGU-1 playing other chips](https://www.youtube.com/playlist?list=PLbSCQdOP-_xh-mkkLIiCdmu_ArqHcOkYM) playlist. To hear it done on demand, [SGU Tracker](https://tracker.x65.zone/) reads the formats those machines were written in and converts each voice into an SGU-1 patch: SID (GoatTracker modules and Rob Hubbard player rips), Amiga MOD and AHX, Impulse Tracker, Scream Tracker 3 and FastTracker 2, AdLib and OPL (RAD, A2M, OPLL instrument ROMs), NES, AY, VGM register logs, Standard MIDI files, and Furnace modules.

Its file dialog browses the online collections in place — the High Voltage SID Collection, Modland, VGMRips, the AdLib archives, Battle of the Bits, Mutopia — so a tune can go from a link in a browser to nine SGU-1 channels without a download step in between. That is the quickest way to find out what the chip does and does not inherit from any particular ancestor.

## Composing for the SGU-1

**SGU Tracker** (<https://tracker.x65.zone/>) is the chip's own tracker, and the shortest path from an idea to a sound. It grew out of klystrack — the same pattern editor, workflow and importers — with the playback engine rewritten to drive an SGU-1 directly, so what the pattern editor plays is what the hardware plays. It is also the best way to learn the operator fields, because its instrument editor names them the way this appendix does. It runs in the browser at that address, and ready-made Linux and Windows builds are published under <https://tracker.x65.zone/latest/>.

**SGM** is the tracker's export format, and the one the X65 actually plays. A `.sgm` module is an ordinary `.xex` chunk stream (see [Appendix F](F_xex_format.md)) carrying the song data, a player image, and — when the module uses samples — a full PCM image in bank 1 that load-time glue blits through the service port. It deliberately carries no reset vector, so loading a module does not run it: music as a library, called from a program's own interrupt handler.

For bring-up and instrument design away from a running X65, the chip also accepts control over SPI and USB MIDI.

## Specification Summary

| Property | Value |
| --- | --- |
| Channels | 9, each stereo via signed pan |
| Operators per channel | 4, freely routed (no fixed algorithms) |
| Waveforms per operator | 8: sine, triangle, sawtooth, pulse, noise, periodic noise, reserved, sample |
| Waveform shaping | 4-bit `WPAR` per operator, waveform-dependent |
| Envelope | AR / DR / SL / SR / RR per operator, 5-bit rates, 7-bit total level, generated at 16 kHz |
| Keying | Level `GATE` plus one-shot `TRIG`, with per-operator key-on delay |
| Filter | State-variable per channel: 16-bit cutoff, 8-bit resonance, low / band / high taps summed |
| Modulation | Per-operator hard sync and 1-bit ring mod, per-channel ring mod from the next channel |
| LFO | Per-channel shape for AM and PM (four each), two depths per operator; fixed rates of 5.86 Hz PM and 2.93 Hz AM |
| Automation | Frequency, volume and cutoff sweeps plus a phase-reset timer, all per channel |
| PCM | 64 KB shared memory, signed 8-bit, per-channel playback or 1024-byte wavetables |
| Output | Stereo 48 kHz I²S into a TI TLV320AIC3254 CODEC, analog line-out |
| CPU interface | 64-byte channel-switched window at `$FEC0–$FEFF` (see [Appendix A](A_memory_map.md)) |
