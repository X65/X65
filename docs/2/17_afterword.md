# Chapter 17: Afterword

## Acknowledgements

The X65 is the work of a small group of people and a much larger collection of shoulders to stand on. A few that deserve particular mention:

- **PCBWay**, for sponsoring the manufacturing of the DEV-boards and delivering them with their characteristic quality.
- **zbyti**, for writing Sokoban in MAD-Pascal and for being the first to put a real game on the platform.
- **Rumbledethumps** and the **picocomputer** project, whose RP6502 / RIA lineage is visible throughout the X65's firmware surface — the fastcall API, the monitor command shape, the HID handling.
- **Andre Weissflog (flooh)**, whose **chips** / chip-emulators toolbox and cycle-accurate 65Cxx core are the technical heart of Emu.
- **tildearrow**, whose **Sound Unit (tSU)** fantasy-chip design is the architectural heart of the SGU-1. Without it, a custom X65 synth chip would have been a much slower and less coherent project.
- **Furmilion**, from the Furnace Tracker community, for patient hands-on help bringing up and tuning the SGU-1 — testing register behavior, reporting quirks, suggesting fixes, and shaping the chip's voice.
- **Android Arts**, whose keyboard layout explorations (Famicube, Amiga770) inspired the X65 keyboard's packed ortholinear top rows.
- Every contributor who has filed a bug, answered a question in the community channels, or written a demo or utility for the DEV-boards.

## Resources

All the X65 project material — code, documentation, community — is linked from the project website. The useful entry points:

| Where                                 | What it is                                                         |
| ------------------------------------- | ------------------------------------------------------------------ |
| <https://docs.x65.zone>               | This book                                                          |
| <https://github.com/X65>              | The project GitHub organisation — firmware, schematic, examples, emulator, K816, cc65-dbg, OS/816, Furnace port |
| <https://x65.zone/emu/>               | Web emulator (WebAssembly build of Emu)                            |
| <https://x65.zone/news.html>          | Development blog — announcements, milestones, deep-dives           |
| Matrix space `#x65:chrome.pl`         | Real-time community chat                                           |

### Core repositories

- **`firmware`** — the authoritative implementation of the NORTH, SOUTH, and audio chips on RP2350 microcontrollers. The behavioural source of truth for this book.
- **`schematic`** — KiCad project and PDF for the DEV-board.
- **`examples`** — cc65 / ca65 starter projects, the `x65.cfg` linker config, and a growing library of example programs per subsystem.
- **`emulator`** — the native and WebAssembly Emu builds.
- **`K816`** — the Rust high-level assembler used by OS/816.
- **`cc65-dbg`** — the VS Code Debug Adapter Protocol extension for cc65 / ca65 workflows.
- **`OS-816`** — the operating system that boots on the X65 by default.
- **`furnace`** — the X65 fork of Furnace Tracker with SGU-1 support.

## Where the Project Is Now

At the time this book snapshot was written, the X65 is in the **Gen2 DEV-board** era: two RP2350s split as a NORTH / SOUTH pair, a dedicated audio RP2350 behind SOUTH, and the Raspberry Pi Radio Module 2 for Wi-Fi and Bluetooth. The SGU-1 sound chip is feature-complete — nine 4-operator FM channels with SID-style filtering, sweeps, and PCM playback, available in Emu and in the Furnace tracker. CGIA provides paletted and attribute modes plus HAM6 and affine, at a perfect 60 Hz over DVI-D. OS/816 has a working scheduler, COP-based syscalls, and a small shell; several of its intended subsystems (CIO, signals, bank allocator, system-wide lock registry) are designed and reserved but not yet implemented.

The long-term shape of the machine is **the full computer**: an all-in-one keyboard form factor with a custom 70 % keyboard (the **🐾** key at the Meta / Windows position), rear and side I/O, extra USB ports, and internal bays for storage — the Amiga / MSX lineage restated in modern parts. The DEV-boards currently in the field are **Milestone 1**: a platform for bring-up, for writing OS/816 and the first generation of software, for proving out each subsystem before they land in the full machine.

Book snapshots trail the code by design — for the current state of the work, the blog, Matrix space, and GitHub organisation linked in the **Resources** table above are the live sources.

## Open-Source Contributions

Every part of the X65 — firmware, OS, emulator, toolchain, this book — is open source and accepts contributions. The usual paths in:

- **Bug reports** on the GitHub repository that owns the behaviour: firmware for chip-level bugs, `OS-816` for kernel bugs, `examples` for example-program bugs, and this book's repository for documentation issues.
- **Pull requests** for fixes or new features — start by opening an issue so the maintainer can flag whether the change fits the project direction before you spend time on it.
- **Example programs** — concrete, small programs that demonstrate one feature well are some of the most valuable contributions, both for the `examples` repository and as blog-post material.
- **Music and graphics assets** for shared demos and games — the SGU-1 + Furnace pipeline and the CGIA + image-converter tooling mean a composer or artist can contribute without touching 65C816 assembly.

If the idea is bigger than a single issue or PR, the Matrix space or Discord is the right place to have the conversation first.

---

That is the end of the book. Thank you for reading — and, if you build something on the X65, please consider sharing it with the community.
