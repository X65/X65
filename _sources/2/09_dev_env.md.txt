# Chapter 9: Development Environment Setup

```{note}
This chapter is still under construction.
```

This chapter covers the concrete steps for going from a blank project directory to a `.xex` running on real X65 hardware. [Chapter 7](../1/7_devel.md) introduced the tools; here we use them.

## Two Kinds of X65 Programs

An X65 binary is always a **`.xex`** file, built with the same toolchain regardless of how it is ultimately going to run. What differs between programs is the runtime environment they target:

- **Boot ROM images.** Loaded by the NORTH firmware from an internal LittleFS boot catalogue immediately after hardware init, before anything else has a chance to run. A boot ROM image has the machine to itself — CPU, memory, every peripheral — and defines the **machine personality**: the face the user sees when they power the board on. In classic-machine terms this is the built-in ROM, the same role the BASIC ROM plays on a C64 or the KERNAL ROM on an Apple II. The **default boot ROM shipped with the X65 is OS/816** (see [Chapter 14](14_os.md)); writing a *different* boot ROM is rare, reserved for developers who want to replace OS/816 and give the machine an entirely different personality.

- **User-level applications.** Launched **under OS/816's control** from its shell — the same shape of `.xex`, the same build pipeline, but the runtime environment is an OS task: a dedicated 64 KB memory bank for the application's code and data, plus a 256-byte direct-page-plus-stack page in bank 0, with hardware access typically going through OS syscalls rather than raw MMIO. Games, utilities, productivity tools — everything a user would reach for once the machine is booted — live here. This is where most day-to-day development lands. Programs that genuinely need the whole machine (demos and similar "takeover" scenarios) are better written as boot ROM images instead.

Both categories are built by the same compiler / assembler / linker pipeline and produce byte-identical `.xex` file structure (see [Appendix F](../A/F_xex_format.md)). Only the *deployment step* and the *runtime assumptions* change, and those are covered below.
