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

## Graphics Terms

**CGIA (Color Graphics Interface Adaptor)** - The X65's graphics subsystem, combining features from Atari ANTIC and Commodore VIC-II/TED architectures.

**Display List** - A structured set of instructions in memory that dictates how each scanline is rendered by the CGIA.

**HAM Mode (Hold-And-Modify)** - A special graphics mode inspired by the Amiga that allows modifying only part of a pixel color per scan, enabling more colors per screen.

**LMS (Load Memory Scan)** - A display list instruction that defines the offset of display memory, holding character or tile data.

**MODE7** - A chunky-pixel graphics mode with hardware-assisted affine transformations, similar to the SNES MODE7.

**Multicolor Mode** - A graphics mode that uses a 4-color-per-cell representation, where each byte encodes four pixels.

**Sprite Multiplexing** - A technique where sprite hardware is reused to display more sprites than hardware natively supports.

**VPU (Video Processing Unit)** - The hardware component that executes display list instructions and updates the screen accordingly.

## Audio Terms

**FM Synthesis** - Frequency Modulation synthesis, the audio generation method used by the Yamaha SD-1 in the X65.

**PWM (Pulse-Width Modulation)** - A technique for generating analog signals from digital devices, used for audio generation in the X65's dual PWM channels.

**SD-1 (YMF825)** - The Yamaha FM synthesizer chip used in the X65 for high-quality music synthesis.

## I/O and System Terms

**DE-9** - The 9-pin connector used for joystick ports in the X65, compatible with Atari-style joysticks.

**DVI-D** - Digital Visual Interface, the video output standard used by the X65.

**ESP32-C3** - The Wi-Fi and Bluetooth module integrated into the X65.

**MMIO (Memory-Mapped I/O)** - A technique where hardware devices are accessed through memory addresses, used extensively in the X65.

**RP2040** - The microcontroller used in the X65 to implement various peripherals and interfaces.

**RP816-RIA (Retro Interface Adaptor)** - The subsystem that provides USB, storage, and other modern I/O capabilities for the X65.

**SPI (Serial Peripheral Interface)** - A synchronous serial communication interface used for connecting various components in the X65.

**UART (Universal Asynchronous Receiver/Transmitter)** - A serial communication protocol used for terminal access and debugging in the X65.
