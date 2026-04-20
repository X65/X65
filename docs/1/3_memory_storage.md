# Chapter 3: Memory and Storage

The X65 gives 65C816 software a flat, uniform, 16 MB address space. There is no bank switching from the programmer's point of view, no ROM overlay to think about, and no special zero page to fight over. Behind that simplicity sits a tiered memory system — PSRAM for bulk, an L2 cache for speed, and a pair of smaller fast areas (direct page, hardware stack, XSTACK) for the idioms the 65C816 rewards. This chapter covers each layer and how it is reached.

## Flat 16 MB Address Space

The main memory is **16 MB of QPI PSRAM**, supplied as two **8 MB banks** running at **112 MHz**. The NORTH chip owns the QPI link and presents a single unified view to the CPU: bank 1 is reached by the high half of the 24-bit address (`$800000` and above), bank 0 by the low half, and the switch between them is done on the fly without 65C816 software intervention.

From the 65C816's point of view, then:

- The `K` (bank) + `A` (16-bit address) pair directly names a byte in PSRAM.
- Long addressing (`lda $123456`) just works across the full range.
- `PBR` and `DBR` move code and data wherever is convenient.

There is no ROM. A small flash chip on the QPI bus stores firmware and boot assets (see **Persistent Storage** below), but from the CPU's side it is not mapped into the 16 MB address space during normal execution.

The only holes in that flat space are the MMIO windows documented in [Appendix A: Memory Map](../A/A_memory_map.md):

- `$FC00–$FCFF` — Expansion slots
- `$FE00–$FFFF` — On-board chip registers (SGU-1, CGIA, timers, LEDs, buzzer, HID, RIA)

Everything else is RAM.

## L2 Cache

PSRAM is not slow in absolute terms, but a serial QPI link still costs tens of nanoseconds per access compared to a few nanoseconds for internal SRAM. NORTH closes that gap with a **64 KB L2 cache** held in the RP2350's internal SRAM. Its sole purpose is to amortise the QPI PSRAM protocol delays so that typical 65C816 access patterns do not pay full PSRAM latency on every access.

From the 65C816's perspective there is nothing to do: the cache is fully transparent. Neither reads nor writes need any special instructions or flags — the CPU just issues normal accesses and NORTH handles the rest. Application code should never need to reason about the cache; it is an implementation detail of the PSRAM path, mentioned here for completeness.

A useful by-product of routing every PSRAM access through the cache is that the two physical 8 MB banks present themselves to the CPU as a single flat 16 MB space without any bank-switching logic visible to software. Chip-select between the two PSRAM parts is decided when a cache line is fetched or flushed — entirely inside the cache-line code path — so bank selection is naturally contained there and the rest of the system does not see it.

## Direct Page

The 65C816's **direct page** is a 256-byte window within bank 0 used by direct-addressing-mode instructions. Unlike the 6502's fixed zero page, direct page is **relocatable** via the `D` (Direct Page) register. Any `D` value in bank 0 is legal; the familiar zero-page style of fast single-byte-operand instructions is simply what you get when `D = $0000`.

On the X65 this is exploited heavily:

- The OS gives each **task** its own direct page, so context switches can move `D` and not clobber other tasks' zero-page state.
- **Interrupt handlers** can have their own direct page set up during dispatch, keeping handler locals separate from the interrupted task's.
- **Modules and libraries** can claim a direct page at load time and never collide with the application.

The programming details of direct-page management are in [Chapter 10: Memory Management](../2/10_memory.md).

## Hardware Stack

In native mode the 65C816's stack pointer `S` is 16 bits wide, so the hardware stack is not architecturally constrained to the 256-byte page the 6502 was stuck with — it can live anywhere within bank 0. The stack instructions also gain full width along with `S`: `PHA`/`PLA` push and pull 16-bit accumulators when the `M` flag is clear, `PHX`/`PHY`/`PLX`/`PLY` follow the `X` flag similarly, and `PEA` / `PEI` / `PER` push address literals and indirect references useful for far calls and structured parameter passing.

How much of that 16-bit range a program actually gets depends on the execution context:

**Bare-metal (software takes over the machine).** When a program takes exclusive control of the X65 — a classic demo, a game that bypasses the OS, a bring-up test — it is free to place the stack wherever in bank 0 makes sense (typically high, growing down) and to use as much of it as the bank can spare. With 64 KB of bank 0 at its disposal and no other tasks to worry about, deep recursion, long interrupt chains, and stack-heavy calling conventions are all comfortably supported.

**OS/816 tasks.** Under OS/816 each task is effectively a small **virtual machine**: a dedicated 64 KB memory bank for the task's own code and data, plus a 256-byte page in bank 0 that holds **both** the task's direct page and its hardware stack. The direct page sits at the bottom of that page and grows upward; the stack sits at the top and grows downward; they meet in the middle:

```text
+------------------+ ← page top:   stack base (S grows down)
|     stack        |
|       ↓          |
|                  |
|       ↑          |
|   direct page    |
+------------------+ ← page bottom: direct page (D points here)
```

The dedicated 64 KB bank is where the task's code, constants, and data live — no different from a bare-metal program's bank 0, just relocated into the task's own slot. The co-located DP-plus-stack page in bank 0 is what makes context switches cheap: one pointer swap and the next task's direct page and stack are in place, with no further memory to walk. The only tight budget is inside that 256-byte page — direct-page footprint and deepest stack depth, added together, must stay comfortably below 256 bytes. Code and data are not part of that budget.

The contrast is worth remembering: the 16-bit stack pointer is a hardware capability of the CPU, but whether a given program has the whole bank 0 to itself or a 64 KB dedicated bank plus a co-located 256-byte page is a question of execution context, not of silicon.

## XSTACK

Alongside the hardware stack, the RIA firmware exposes a **512-byte auxiliary buffer called XSTACK** — a last-in-first-out region that lives inside NORTH, not in CPU-visible PSRAM. Its job is to carry arguments and return data between 65C816 code and the fastcall API at `$FFF0–$FFF3` without clobbering the hardware stack or forcing callers to pass pointers into PSRAM.

A typical fastcall pattern writes a few bytes to the XSTACK, sets up `A`/`X`/`Y`, writes the call opcode to `$FFF1`, and on return reads the result either from registers or back off the XSTACK. From the application programmer's point of view the XSTACK is just another stack, reachable through RIA registers; its benefit is that it is NORTH-local, so reading and writing it does not stall on PSRAM.

## Memory-Mapped I/O Window

All the custom chips live in a small window at the top of bank 0:

| Range         | Device                             |
| ------------- | ---------------------------------- |
| `$FC00–$FCFF` | Expansion slots (16 × 16 bytes)    |
| `$FEC0–$FEFF` | SGU-1 sound                        |
| `$FF00–$FF7F` | CGIA graphics (128 registers)      |
| `$FF80–$FF97` | GPIO expander (reserved)           |
| `$FF98–$FF9F` | System timers                      |
| `$FFA0–$FFA7` | RGB LEDs                           |
| `$FFA8–$FFAB` | System buzzer                      |
| `$FFB0–$FFBF` | USB HID                            |
| `$FFC0–$FFFF` | RIA (including fastcall + vectors) |

Reads and writes to these ranges are diverted by NORTH to the relevant chip or PIX device rather than going to PSRAM. Every MMIO access is a real transaction against the target device.

Because MMIO shares the same address space as RAM, the usual 65C816 idioms apply: `lda $FFB1` reads the HID data byte, `sta $FFA0` lights up LED 0. The byte-level layout of each window is documented in [Appendix A: Memory Map](../A/A_memory_map.md); detailed programming sequences for each subsystem are covered in Part 2.

## Persistent Storage

The X65 carries a **flash chip** on the same QPI bus as PSRAM. It is used internally by the NORTH firmware for its own configuration and for a **LittleFS**-formatted store of **boot ROM images** — snapshots of 65C816 code that the firmware loads into PSRAM at start-up or on demand. The LittleFS volume is an internal detail of the firmware; it is **not** exposed to the 65C816 as a filesystem and 65816 software cannot `open` / `read` files from it.

User-accessible storage comes from **USB Mass Storage** devices — pen drives, SD-card readers, external SSDs — plugged into the X65's USB port. The USB host is implemented on the **NORTH RP2350** using its own USB interface running in host mode; NORTH exposes the mounted volume as a FatFS filesystem reached through the fastcall API at `$FFF0–$FFF3` (`open` / `read` / `write` / `close` idioms). This is the path applications use for loading program data, saving game state, and generally putting user files in front of 65C816 software.

The practical division of labour, then: flash holds the boot/runtime images the firmware itself needs; USB is where everything an application or user touches lives.
