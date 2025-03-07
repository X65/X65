# Chapter 5: Sound and Audio

## Yamaha SD-1 (YMF825) FM Synthesizer

The **Yamaha SD-1 (YMF825)** serves as the primary FM synthesis engine of the X65 microcomputer, delivering rich, multi-voice sound with minimal CPU overhead. This **hardware FM synthesizer** is based on Yamaha’s extensive experience with frequency modulation synthesis, offering a balance between **sound quality and low processing demands**.

### Features of the SD-1 FM Synthesizer

- **4-operator FM synthesis**, enabling complex sound generation similar to later Yamaha FM chips.
- **16-channel polyphony**, allowing multiple simultaneous sounds.
- **29 on-chip operator-waveforms** and **8 algorithms**, offers a whole variety of

  sound.

- **Built-in effects processing**, including envelope shaping and vibrato.
- Integrated **3-band equalizer**.

### Integration with the X65

The SD-1 is connected to the system via an SPI interface, which is memory-mapped by RIA hardware, allowing efficient access by 65816 CPU. All SD-1 registers are directly accessible in a dedicated memory region, allowing direct programming. Since the SD-1 operates as a **standalone digital synthesizer**, the CPU only needs to send **configuration commands**, freeing resources for game logic and other tasks.

### Mono Output and Mixing

The YMF825 produces a **mono digital audio signal**, which is then processed by the X65’s **integrated audio mixer**. This ensures that FM synthesis is seamlessly combined with other sound sources, such as the **PWM audio channels**.

---

## Dual PWM Audio Channels

In addition to the FM synthesizer, the X65 features **two Pulse-Width Modulation (PWM) audio channels**, which serve as a flexible alternative for generating sound effects and basic waveform synthesis. These PWM channels are directly controlled by the CPU, making them ideal for **custom waveform generation, simple music playback, and sampled sound effects**.

### Key Features of the PWM Audio System

- **Two independent channels**, allowing stereo-like effects through software control.
- **8-bit duty cycle**, allowing dynamic sound shaping.

### How PWM Works in the X65

Each PWM channel is controlled by a **dedicated hardware timer**, which determines the duty cycle and frequency of the generated waveform. By rapidly toggling the output voltage, PWM creates a square wave that can be manipulated to **approximate analog signal**.

While the **SD-1 handles complex musical synthesis**, the PWM channels provide an **method for generating simple square wave, percussive sounds, and sampled audio effects**. When combined, these audio subsystems create a **versatile sound environment** that balances **hardware efficiency and creative flexibility**.

### Applications of PWM Audio

- **Simple programmatic beeps**: Useful for system notifications, debugging signals, or simple chiptune-style sound effects.
- **Basic waveform synthesis**: By carefully adjusting the duty cycle, PWM can simulate square, triangle, and other waveforms.
- **Advanced sound generation**: Through precise timing and software control, complex sounds can be synthesized, including dynamically changing waveforms and amplitude modulation techniques.
- **Speech synthesis and sampled audio**: With clever programming, PWM can reproduce low-resolution sampled sounds for speech effects or digital audio playback.

---

## Integrated Mixer

The X65 features a **hardware-integrated audio mixer**, responsible for **combining and balancing** the various sound sources within the system. The mixer ensures that both **FM synthesis (YMF825), PWM audio, and External Audio-In** are properly blended before reaching the output.

### Mixer Capabilities

- **Combines SD-1 FM audio and dual PWM channels** into a single output stream.
- **Adjustable volume levels** for each source, allowing software-controlled mixing.
- **Simple low-pass filtering**, improving sound quality by smoothing out PWM artifacts.
- **Stereo output support**, ensuring flexibility in speaker or headphone configurations.
- **Integrated gain control**, preventing distortion and maintaining dynamic range.
- **Programmable audio routing**, allowing different mixing configurations depending on system settings.

### Summary

The **X65 sound system** combines **hardware FM synthesis, PWM-generated sound, and an integrated mixer** to provide a robust and flexible audio solution. The **Yamaha SD-1 delivers high-quality FM music**, the **dual PWM channels offer custom waveform generation**, and the **hardware mixer** seamlessly blends all sources for a **cohesive and powerful sound experience**.
