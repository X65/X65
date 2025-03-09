# Chapter 2: System Architecture Overview

```{note}
This chapter is still under construction.
```

## CPU: The 65C816 in Native Mode

## Memory Map and Addressing

## Interrupt Handling and Priority System

## System Clock and Performance

The X65 microcomputer is built around the **WDC 65C816** CPU, a 16-bit processor with a 24-bit address space capable of addressing up to 16MB of memory. To efficiently interface this CPU with modern components, the X65 employs innovative bus circuitry and memory optimization techniques that enhance system performance.

### CPU Bus Circuitry

The 65C816 CPU utilizes a multiplexed address and data bus to manage its extensive address space without requiring additional physical pins. Specifically, during the first (LOW) half of the PHI2 clock cycle, the data bus pins (`D0-D7`) transmit the upper address bits (`A16-A23`). In the second (HIGH) half, these pins switch to handle data read/write operations. This design necessitates latching circuitry to capture the upper address bits during the appropriate clock phase.

To accommodate this mechanism, the X65 leverages a design inspired by the Neo6502 computer, employing three buffers connected to the address (`A0-A15`) and data (`D0-D7`) lines. This configuration effectively multiplexes the 24 CPU lines into 8 GPIO lines of the RP2040 microcontroller, reducing pin requirements and simplifying the interface. The RP2040's Programmable I/O (PIO) program reads the full address during the LOW phase of PHI2, swiftly switches buffer control signals, and then manages data read/write operations during the HIGH phase. This approach enables the RP2040 to emulate RAM, Video Processing Unit (VPU), and I/O devices across the entire 24-bit address space.

### VAB Memory Cycle Optimization

To further enhance performance, the X65 implements a memory cycle optimization technique based on the 65C816's Valid Data Address (VDA) and Valid Program Address (VPA) signals. These signals indicate when the CPU is accessing data or program memory, respectively, which occurs in approximately 70% of the CPU's cycles. During the remaining cycles, the CPU performs internal operations that do not involve memory access.

By combining the VDA and VPA signals into a single Valid Address Bus (VAB) signal, the X65 can detect cycles where the CPU does not require memory access. In these instances, the system can drive the PHI2 clock without waiting for a memory round-trip, effectively shortening the cycle time for internal CPU operations. This optimization reduces the impact of memory bus latency on overall CPU performance, resulting in a more efficient execution of instructions.
