# Chapter 8: Introduction to 65816 Assembly

## Overview of the 65C816 CPU

The **WDC 65C816** (commonly referred to as the 65816) is a versatile 16-bit microprocessor developed by the Western Design Center (WDC). Building upon the foundation of the 8-bit 6502 and its CMOS variant, the 65C02, the 65816 introduces several enhancements that make it suitable for more advanced computing applications. citeturn0search23

### Key Features

- **16-Bit Registers:** The 65816 features 16-bit registers, allowing for more efficient data handling and processing compared to its 8-bit predecessors.

- **Expanded Memory Addressing:** With 24-bit address lines, the 65816 can directly address up to 16 megabytes of memory, a significant increase from the 64KB limit of the 6502.

- **Enhanced Instruction Set:** The processor includes an expanded set of instructions and addressing modes, providing greater flexibility and efficiency in programming.

- **Dual Operating Modes:** The 65816 operates in two modes:

  - **Emulation Mode:** Upon reset, the CPU starts in emulation mode, behaving like a 65C02 to maintain compatibility with existing 8-bit software.

  - **Native Mode:** Switching to native mode unlocks the full 16-bit capabilities and extended features of the processor.

### Common Misunderstandings

Despite its advancements, several misconceptions about the 65816 persist:

- **16-Bit Operations Exclusivity:** A prevalent myth is that native mode restricts the CPU to 16-bit operations. In reality, programmers can dynamically choose between 8-bit and 16-bit operations for the accumulator and index registers, offering fine-grained control over data processing.

- **Complex Bus Utilization:** Some believe that leveraging the 65816's extended memory requires intricate hardware configurations. However, for applications confined to a 64KB address space, the additional bank address byte can be disregarded, simplifying system design.

- **Direct Page Register Limitations:** There's a misconception that the Direct Page (D) register is limited in flexibility. In truth, the 16-bit D register allows the direct page to start at any address within the first 64KB, and it doesn't need to be page-aligned. This feature enables efficient and flexible memory management.

- **Minimal Enhancements Over 6502:** Some view the 65816 merely as a wider 6502, overlooking its numerous new instructions and addressing modes, such as stack-relative addressing, which enhance its capability for complex tasks like multitasking and interrupt handling.

### Applications

The 65816's robust feature set has led to its adoption in various computing systems, including:

- **Apple IIGS:** An advanced model in the Apple II series, utilizing the 65816 for enhanced performance and capabilities.

- **Super Nintendo Entertainment System (SNES):** The Ricoh 5A22 CPU, based on the 65816, powers this iconic gaming console, enabling complex graphics and gameplay mechanics.

- **Acorn Communicator:** A business computer developed by Acorn Computers, leveraging the 65816's features for improved functionality.

Understanding the 65816's architecture and capabilities is crucial for effectively programming the X65 microcomputer, which harnesses this processor to deliver a powerful and flexible computing platform.

## Registers and Addressing Modes

## Transitioning from 6502 to 65816
