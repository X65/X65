# Chapter 5: Sound and Audio

## The SGU-1 Synthesizer

The X65's primary sound source is the **SGU-1 (Sound Generator Unit 1)** — a custom hybrid synthesis chip designed for the X65 rather than an off-the-shelf part. Contemporary advanced synth chips are no longer manufactured; rather than constrain the project to whatever vintage silicon is still sourceable, the X65 builds its own. In one line: each voice does **Yamaha-style FM for additive sound creation**, then runs the result through a **SID-style filter for subtractive sculpting**. Each of its **nine channels** therefore behaves like a small modular synth: build the timbre with FM, shape it with a multimode filter, animate it with sweeps, and optionally swap any operator's waveform for a PCM sample.

Conceptually the SGU-1 sits closer to ESFM and OPZ than to plain OPL3, and it takes its operator model from **tildearrow's Sound Unit (tSU)** — the fantasy synth chip from the Furnace tracker — scaled up and wrapped in a memory-mapped register interface. It is its own design with its own register layout. Concrete specs:

* **9 channels**, each producing **stereo** output via a per-channel pan setting.
* **4 operators per channel**, freely routed (any operator can be a modulator, a carrier, or both).
* **48 kHz** internal sample rate.
* **64 KB of shared PCM RAM** for samples.

## Channels and Operators

Every channel owns four operators numbered 0–3. The **MOD** and **OUT** fields on each operator decide what flavour of FM topology you actually get: setting only `OUT` on operator 3 and chaining `MOD` 3←2←1←0 gives the classic 4-op stack; setting `OUT` on every operator gives an additive/mix structure; mixing the two reproduces the OPL-style 2-op pairs and most OPN/OPM-ish 4-op algorithms. Operator 0’s `MOD` field doubles as a **feedback gain**, just as in ESFM.

Pitch is set once per channel (the 16-bit `FREQ` register). Each operator derives its own frequency from that base via an **OPL-style multiplier (`MUL`)** plus a 3-bit **detune (`DT`)**. An operator can also opt out of channel pitch entirely by setting the **`FIX` bit**, after which `MUL` and `DT` together encode an absolute frequency in the OPZ tradition — useful for percussion partials and metallic timbres.

## Waveforms

Each operator independently chooses from **eight waveform slots**:

| Code | Waveform         | Notes                                                                                                            |
| ---- | ---------------- | ---------------------------------------------------------------------------------------------------------------- |
| 0    | `SINE`           | OPL-style shape variants via `WPAR` (half-sine, abs-sine, quantized).                                            |
| 1    | `TRIANGLE`       | Same `WPAR` modifiers as sine.                                                                                   |
| 2    | `SAWTOOTH`       | Same `WPAR` modifiers as sine.                                                                                   |
| 3    | `PULSE`          | Pulse width either taken from the channel `DUTY` register or fixed per-operator (1/16 … 15/16).                  |
| 4    | `NOISE`          | 32-bit LFSR white noise, clocked SID-style from the channel frequency.                                           |
| 5    | `PERIODIC_NOISE` | 6-bit LFSR with four selectable tap configurations — covers the metallic/tonal noise palette of POKEY and SID.   |
| 6    | *(reserved)*     | —                                                                                                                |
| 7    | `SAMPLE`         | A 1024-sample slice of the shared PCM RAM, played as if it were a wavetable; phase wraps naturally.              |

Because waveform selection is **per-operator**, a single channel can mix, say, a noise modulator into a sine carrier, or use a sampled instrument as the FM modulator for a pulse carrier.

## ADSR Envelopes

Each operator has its own envelope generator with independent **Attack, Decay, Sustain Rate, Sustain Level, and Release** parameters (`AR`, `DR`, `SR`, `SL`, `RR`). The `AR`, `DR`, and `SR` rates are 5 bits wide for finer control than the 4-bit OPL equivalents, and the explicit second-decay rate (`SR`, sometimes called `D2R`) gives much better percussive shapes than OPL’s implicit “sustain off” mode.

Envelope state advances at 16 kHz (one third of the audio rate). Key-on can be delayed per-operator by `2^(DELAY+8)` samples, which is enough to choreograph multi-operator attack stacks without CPU intervention.

## Per-Channel Multimode Filter

After the four operators are summed into a channel mix, the signal passes through a **state-variable filter** with cutoff and resonance — the same architecture made famous by the SID. The channel `FLAGS0` register chooses which combination of low-pass, high-pass, and band-pass outputs to tap, so a channel can be configured as any of LP / HP / BP, or as compound modes that mix two outputs at once.

Cutoff is 16 bits and resonance is 8 bits. Both are valid sweep targets (see below), so filter motion needs no sample-rate CPU work.

## Sweeps

Each channel carries three independent **hardware sweep generators** that step a target value at a programmable rate:

* **Frequency sweep** — pitch slides, vibrato envelopes, arpeggios.
* **Volume sweep** — tremolo, attack/decay shaping outside the operator envelope, fades.
* **Cutoff sweep** — filter motion that follows the note rather than a global LFO.

Each sweep has a 16-bit `SPEED`, an 8-bit `AMOUNT` (which carries direction and mode flags), and an 8-bit `BOUND` that limits travel and chooses wrap-around or bounce behaviour. Once enabled via the matching bit in `FLAGS1`, sweeps run autonomously alongside the operator envelopes.

## PCM Playback

Any channel can be put into **PCM mode** (`FLAGS0` bit 3) and then plays back from the shared 64 KB PCM RAM instead of running its FM operators. Three pointers — current position, end boundary, and loop restart — control playback; setting the `PCM_LOOP` bit in `FLAGS1` turns one-shots into loops at the loop-restart point.

PCM samples are signed 8-bit. The same PCM region is also reachable from the operator side via the `SAMPLE` waveform, so a sound can use a slice of PCM as a wavetable while another channel streams a longer recording from a different region.

## Audio Plumbing on the X65

The SGU-1 runs on its own **dedicated audio chip** — a separate RP2350 attached to the SOUTH chip over an **SPI** link. From the CPU, writes to the SGU register window at **`$FEC0–$FEFF`** are picked up by the NORTH chip and sent over the [PIX bus](../A/B_glossary.md) as `SPU`-device messages; SOUTH receives them and forwards the traffic over SPI to the audio chip, where the SGU-1 firmware actually handles the write. The audio chip renders the mix at **48 kHz** stereo and pushes it out over **I²S** to an on-board audio CODEC, which converts to analog stereo line-out. The same CODEC is driven by firmware over I²C for gain, mute, and routing.

The CPU window exposes **all registers of a single channel at once**. The last register in the window (`$FEFF`) acts as a **channel-index switch**: writing a channel number there remaps the rest of the window to that channel's registers. A reserved channel index `0xFF` swaps in service registers — chip identification, version, and mixer/DSP controls — instead of a sound channel.

## System Buzzer

Alongside the SGU-1 the X65 carries a simple **PWM-driven buzzer** intended for system beeps, attention tones, and the kind of click feedback an OS likes to make. It is reached not through the SGU window but as a `MISC` device on the PIX bus, with two commands (set frequency, set duty cycle). It is intentionally minimal — for music, samples, and sound effects, use the SGU-1.

## Composing for SGU-1

A **Furnace Tracker** port maintained at `github.com/X65/furnace` adds SGU-1 as a first-class supported chip. Songs can therefore be authored in a modern tracker workflow, previewed against the actual hardware model, and exported into formats usable from 65816 code. For programmers working directly in assembly, [Chapter 12: Sound Programming](../2/12_sound.md) covers the idiomatic register sequences for key-on, envelope, filter, sweep, and PCM playback.

## Summary

The X65's sound system centres on the custom **SGU-1**: nine stereo voices that combine four-operator FM with a SID-style multimode filter, eight per-operator waveforms (including PCM-as-wavetable and configurable periodic noise), per-operator ADSR envelopes, and three independent hardware sweeps per channel. A 64 KB shared PCM bank lets any channel switch from synthesis to sample playback at will. The whole engine runs on a dedicated audio co-processor, reaches the CPU through the PIX bus and a windowed register file, and is supported end-to-end by a Furnace Tracker port. A small system buzzer handles the housekeeping tones the SGU-1 should not have to bother with.
