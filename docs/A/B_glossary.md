# Appendix B: Glossary of X65 Terms

## CPU and Memory Terms

**65C816** - The Western Design Center 16-bit microprocessor that serves as the CPU for the X65, offering backward compatibility with the 6502 while providing 16-bit capabilities and a 16MB address space.

**Accumulator** - The primary register used for arithmetic and logical operations in the 65C816 CPU.

**Bank** - A 64KB segment of memory in the 65C816's address space. The X65 supports 256 banks for a total of 16MB.

**Direct Page** - A 256-byte region of memory that can be located anywhere in bank 0, offering faster access for certain instructions (similar to the Zero Page in 6502 processors).

**Emulation Mode** - A compatibility mode of the 65C816 where it behaves like a 65C02 with an 8-bit accumulator and 16-bit address space.

**Native Mode** - The full 16-bit operating mode of the 65C816, which the X65 uses exclusively from boot-up.

**PSRAM** - Pseudo Static Random-Access Memory, the type of memory used in the X65 to provide 16MB of RAM with low power consumption.

**VAB (Valid Address Bus)** - A signal derived from the 65C816's VDA and VPA signals, used to optimize memory cycles when the CPU is performing internal operations.

**XSTACK** - A 512-byte auxiliary buffer maintained by the RIA firmware. Used by the OS fastcall API to pass call arguments and return data between 65816 software and the NORTH chip without touching the 6502 hardware stack.

## Graphics Terms

**CGIA (Color Graphics Interface Adaptor)** - The X65's graphics subsystem, combining features from Atari ANTIC and Commodore VIC-II/TED architectures.

**Display List** - A structured set of instructions in memory that dictates how each scanline is rendered by the CGIA.

**Affine Mode (MODE7)** - The CGIA's mode 7: a chunky-pixel graphics mode with hardware-assisted affine transformations, similar to the SNES MODE7.

**HAM6 (Hold-And-Modify, mode 6)** - A CGIA graphics mode inspired by the Amiga HAM mode. Uses 6-bit commands packing four screen pixels into three bytes; commands either select a base color or modify a single R/G/B channel of the previous pixel by a signed delta, enabling many more on-screen colors at the cost of per-pixel precision.

**LMS (Load Memory Scan)** - A display list instruction that defines the offset of display memory, holding character or tile data.

**MODE7** - Common name for the CGIA's affine mode (mode 7). See *Affine Mode*.

**Multicolor Mode** - A per-instruction flag bit on CGIA text and bitmap modes that switches to a 4-color-per-cell representation, where each byte encodes four pixels.

**Plane Order Register** - A single CGIA register whose value selects one of 24 Z-order permutations of the four planes (using Steinhaus-Johnson-Trotter ordering). Lets a program shuffle which plane is on top — typically from a raster interrupt — without rebuilding any plane.

**Sprite Multiplexing** - A technique where sprite hardware is reused to display more sprites than hardware natively supports.

**VPU (Video Processing Unit)** - The hardware component that executes display list instructions and updates the screen accordingly.

## Audio Terms

**FM Synthesis** - Frequency Modulation synthesis, the per-operator sound generation method at the heart of the X65's SGU-1 chip.

**PWM (Pulse-Width Modulation)** - A technique for generating analog signals from digital devices. Used by the X65 system buzzer to produce simple tones.

**SGU-1 (Sound Generator Unit 1)** - The X65's custom synthesis chip. Nine stereo channels of four-operator FM with per-operator waveform selection (including PCM as a wavetable), per-operator ADSR envelopes, a SID-style multimode filter, three independent hardware sweeps per channel, and a shared 64 KB PCM bank. Implemented in firmware on a dedicated RP2350 audio chip that is attached to the SOUTH chip over an SPI link; reached from the CPU as the `SPU` device on the PIX bus, which SOUTH forwards over SPI to the audio chip.

## I/O and System Terms

**DE-9** - The 9-pin connector used for joystick ports in the X65, compatible with Atari-style joysticks.

**DVI-D** - Digital Visual Interface, the video output standard used by the X65.

**ESP32-C3** - The Wi-Fi and Bluetooth module integrated into the X65.

**MMIO (Memory-Mapped I/O)** - A technique where hardware devices are accessed through memory addresses, used extensively in the X65.

**NORTH chip** - The RP2350 microcontroller in the X65 that owns the CPU bus, the PSRAM interface, and system services (RIA). Communicates with the SOUTH chip over the PIX bus.

**PIX (Pico Information eXchange) bus** - The dedicated serial-with-handshake bus connecting the NORTH and SOUTH chips. Carries memory writes, DMA transfers, and per-device commands to the VPU (CGIA), SPU (SGU-1), and other south-side peripherals.

**RIA (Retro Interface Adaptor)** - The subsystem inside the NORTH chip that exposes the fastcall API at `$FFF0–$FFF3`, aggregates IRQ sources into a single CPU interrupt, and provides USB, storage, and other modern I/O capabilities to 65816 software.

**RP2350** - The Raspberry Pi microcontroller used in the X65 to implement the NORTH and SOUTH chips. Its Programmable I/O (PIO) blocks are how the X65 handles the CPU bus, the PIX bus, and several peripheral protocols in software.

**SOUTH chip** - The RP2350 microcontroller in the X65 that hosts the CGIA (graphics) implementation together with the terminal and font systems. Also bridges the separate SGU-1 audio chip via an SPI link, presenting it as the `SPU` device on the PIX bus. Reached from the CPU via the PIX bus.

**SPI (Serial Peripheral Interface)** - A synchronous serial communication interface used for connecting various components in the X65.

**UART (Universal Asynchronous Receiver/Transmitter)** - A serial communication protocol used for terminal access and debugging in the X65.
