# Chapter 1: Introduction

## The Vision Behind the X65

The modern computing landscape has become increasingly complex, often overwhelming both new learners and experienced developers alike. The X65 microcomputer was created as a response to this trend - a return to an era when computers were understandable and accessible.

The **X65** microcomputer is a modern reimagining of classic 8-bit computing, designed to blend the simplicity and charm of vintage home computers with the power and flexibility of contemporary hardware. Unlike traditional retro-inspired projects that focus on strict authenticity, the X65 embraces modern design choices while retaining the essence of classic computing: direct hardware control, assembly programming, and a practical approach to understanding low-level system architecture.

The core philosophy behind the X65 is to **create an 8-bit-style computer with a clean, modern implementation**, prioritizing ease of expansion, real-time operation, and developer accessibility. With a **65C816 CPU running in native mode**, a **flat 16MB RAM architecture**, and a suite of powerful peripherals including the custom **SGU-1 synthesis chip** and a **DVI-D output capable of 256-color graphics**, the X65 is both an educational platform and a fully capable development machine for 65816 assembly programming.

## Why the 65816?

The **WDC 65C816** was chosen as the heart of the X65 due to its backward compatibility with the classic 6502 while offering significant improvements, including:

- **16-bit capabilities** while maintaining an 8-bit data bus
- **A full 16MB address space** (compared to the 64KB of the 6502)
- **Native mode operation from boot-up**, assuming the CPU is always running in this mode and ignoring emulation mode entirely
- **Efficient stack and direct page handling**, allowing flexible memory management

Unlike traditional 8-bit micros that required bank switching to access more memory, the **X65 takes full advantage of the 65816's flat memory model**, making it an ideal platform for larger programs and modern applications.

## Design Philosophy: Retro Aesthetics with Modern Usability

The X65 is built on the idea that computing should not be a struggle against unnecessary complexity. In a world where software stacks have grown bloated and opaque, this project reclaims the spirit of early computing, where users had full control and could directly understand how their systems worked.
The X65 is **not** a direct clone of any vintage system. Instead, it builds upon the **best ideas from past home computers**, integrating modern features while maintaining a low-level programming experience. Some of the key design principles include:

- **Direct hardware access:** Minimal operating system overhead; programs interact directly with hardware.
- **Rich I/O options:** Supporting classic peripherals (DE-9 joysticks) and modern devices (USB HID keyboards/gamepads, Wi-Fi, and USB mass storage).
- **Advanced graphics and sound:** A beam-chased display system driven by a dedicated display list in CGIA hardware, along with the custom SGU-1 chip for FM synthesis, PCM playback, and SID-style filtering.
- **Open and expandable:** A full CPU bus expansion connector allows custom hardware to interface seamlessly.

### DEV-board and the Full Machine

The X65 ships first as a **DEV-board** — a development platform that validates the hardware and gives programmers, game authors, and OS/816 contributors a working target to write against. The DEV-board is intentionally positioned as **Milestone 1**: it is the complete computer architecturally, but physically it is a bare PCB meant for the lab bench, not the living room.

The long-term direction is a **full-machine, all-in-one keyboard computer**: a custom 70% keyboard, rear/side I/O ports arranged in the familiar DEV-board layout plus additional USB ports, and internal bays for storage in the tradition of the Amiga and MSX. Everything in this book applies to both form factors — the hardware is the same — but the full machine should be understood as the eventual public-facing home of the project rather than its current shape.

## Who Is This Computer For?

The X65 is designed for **hobbyists, retro enthusiasts, and developers** who appreciate the simplicity and direct control of classic computing but want a platform that removes many of the limitations of vintage hardware. It is ideal for:

- **Assembly programmers** who want to write low-level, highly optimized code.
- **Developers of retro-inspired games and demos** who need access to powerful, yet simple, graphics and sound.
- **Students and educators** looking for an approachable way to learn about system architecture and low-level programming.
- **Hardware hackers and DIY builders** who want a computer with direct access to a well-documented expansion bus.

## What This Book Covers

This book is divided into two main parts:

1. **The Hardware and Architecture of the X65** – A deep dive into the computer’s design, including its CPU, memory, graphics, sound, and I/O subsystems.
2. **Programming the X65 in 65816 Assembly** – A practical guide to writing software for the system, covering everything from basic assembly syntax to advanced optimization techniques.

By the end of this book, you will have a complete understanding of how the X65 works **at both the hardware and software levels**, enabling you to write your own applications, develop new hardware expansions, or even modify the system to fit your needs.

## Looking Ahead

In the next chapter, we will explore the **system architecture** of the X65, detailing how its various components interact and what makes it unique compared to both vintage and modern systems.
