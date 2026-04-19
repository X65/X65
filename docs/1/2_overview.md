# Chapter 2: System Architecture Overview

The X65 microcomputer is built around the **WDC 65C816** CPU, a 16-bit successor to the 6502 with a 24-bit address space capable of reaching up to 16 MB of memory. Surrounding it is a small set of custom peripheral chips — **CGIA** for graphics, **SGU-1** for sound, and **RIA** for system services and interrupt aggregation — each implemented in firmware on Raspberry Pi **RP2350** microcontrollers. The system is organised as a **NORTH** chip (CPU bus, memory, system services) and a **SOUTH** chip (graphics, terminal), connected by a dedicated inter-chip bus called **PIX**; a third RP2350 hosts the SGU-1 audio chip and hangs off SOUTH over an SPI link.

## CPU: The 65C816 in Native Mode

The X65 operating system boots the 65C816 into **native mode** right away, where its full 16-bit data path and 24-bit address bus are exposed to software. In native mode the accumulator (`A`), index registers (`X`/`Y`), and stack pointer (`S`) can be switched to 16-bit; a **direct page register (`D`)** lets zero-page-style addressing be relocated anywhere in bank 0; and two bank registers — **program bank (`PBR`)** and **data bank (`DBR`)** — extend addressing across the full 16 MB.

Legacy 6502 code generally does **not** need a mode switch: the 65C816 in native mode is a full superset of the 65C02 instruction set, so existing 6502/65C02 routines execute as-is alongside native-mode code. Switching back into **emulation mode** via `XCE` is technically possible, but it is discouraged and not supported by the project — the X65 firmware, OS, and toolchain all assume native mode throughout. For the full enhanced register and instruction set, see [Appendix C: 65C816 Migration](../A/C_65816_migration.md) and [Appendix E: Cheat Sheet](../A/E_cheat_sheet.md).

## CPU Bus Circuitry

The 65C816 multiplexes its address and data bus to expose a 24-bit address space without growing its physical pin count. During the **LOW** half of the `PHI2` clock cycle, the data pins (`D0–D7`) carry the upper address bits (`A16–A23`); during the **HIGH** half they switch to data read/write.

The **NORTH** chip bridges the CPU bus directly: all 24 CPU address/data lines plus the `VAB` and `RWB` control signals are wired to dedicated RP2350 GPIO pins. A Programmable I/O (PIO) program sequences the two halves of the cycle — sampling the upper address bits `A16–A23` off the multiplexed data pins during the LOW phase of `PHI2`, then servicing the actual read or write on those same pins during the HIGH phase. Architecturally NORTH plays the same role a modern PC's **northbridge** plays for an x86 CPU: it isolates the CPU from differently-clocked memory and peripherals, and routes each access either to the 16 MB PSRAM or to the correct on-system chip — the Video Processing Unit (CGIA), the Sound Generator Unit (SGU-1), the RIA, or the `$FE`/`$FF` pages MMIO devices — across the full 24-bit address space.

### VAB Memory Cycle Optimisation

The 65C816 exposes two signals — **VDA** (Valid Data Address) and **VPA** (Valid Program Address) — that mark cycles in which the CPU is actually accessing memory. Combined into a single **Valid Address Bus (VAB)** signal, they identify the roughly 30 % of cycles in which the CPU is doing internal work and does not need the bus.

On those cycles the X65 can advance `PHI2` immediately, without waiting for a memory round-trip. This shortens internal CPU cycles to the minimum the 65C816 can sustain and meaningfully raises effective throughput compared with a naive bus interface that pays full memory latency on every cycle.

### L2 Cache

PSRAM is fast, but the serial QPI link to it still costs tens of nanoseconds per access. NORTH mitigates this with a **64 KB direct-mapped L2 cache** kept entirely in the RP2350's internal SRAM, which runs at full MCU speed with no wait states. The cache is organised as 2048 lines of 32 bytes; a line's index is taken from middle address bits and its tag from the remaining high bits. A hit services the CPU read or write directly from SRAM; a miss performs a single PSRAM line fetch.

64 KB is enough to hold the working set of a typical task — a BASIC program, a game level, an OS component — which is enough to keep hit rates high during normal execution. The firmware exposes a compile-time switch between this software L2 and the RP2350's hardware QSPI XIP cache (16 KB but slightly faster per hit) so the two approaches can be compared on real workloads.

## Memory Map and Addressing

The X65’s main memory is **16 MB of QSPI PSRAM** organised as two 8 MB banks, switched on the fly by the NORTH chip when the CPU crosses the bank boundary at address `$800000`. From the CPU’s perspective this is a single flat 24-bit address space.

Inside bank 0, the high-`$FE`/`$FF` pages window is reserved for memory-mapped I/O. The canonical layout is:

| Range         | Device                                                            |
| ------------- | ----------------------------------------------------------------- |
| `$FEC0–$FEFF` | SGU-1 (sound)                                                     |
| `$FF00–$FF7F` | CGIA (graphics, 128 registers)                                    |
| `$FF80–$FF97` | TCA6416A GPIO expander (joysticks)                                |
| `$FF98–$FF9F` | System timers                                                     |
| `$FFA0–$FFA7` | RGB LED                                                           |
| `$FFA8–$FFAB` | Buzzer                                                            |
| `$FFB0–$FFBF` | HID (keyboard / mouse)                                            |
| `$FFC0–$FFFF` | RIA registers, including the fastcall API window at `$FFF0–$FFF3` |

Expansion devices live in a separate window starting at `$FC00` (one slot is sixteen bytes; for example, an optional OPL-3 sits at `$FC00–$FC1F`). The full address table — including every RIA sub-register and CGIA plane register — is maintained in [Appendix A: Memory Map](../A/A_memory_map.md).

The 65C816’s **direct page** can be repointed anywhere in bank 0 with the `D` register, which makes it possible to give different routines, tasks, or interrupt handlers their own private fast-access window without disturbing each other. The hardware **stack** lives in bank 0 as well, and the firmware implements a small auxiliary buffer called the **XSTACK** (512 bytes) that the OS uses to pass call arguments between the 65816 and the RIA when invoking system services.

## Interrupt Handling and Priority System

The X65 has two interrupt lines visible to the 65C816 — the maskable `IRQ` and the non-maskable `NMI` — plus the `RES` reset line. The 65C816 dispatches each through its native-mode vector table at the top of bank 0 (`$FFE4–$FFFF`).

Rather than wiring every device directly to the CPU, all maskable sources are funnelled through an **interrupt controller** inside the **RIA**. The RIA aggregates interrupts from peripherals such as the GPIO expander (joystick edges), system timers, the HID subsystem, and external expansion slots, and asserts a single `IRQ` line to the CPU. Software reads RIA status registers in the `$FFC0–$FFFF` window to discover which source fired.

The joystick GPIO expander is an NXP **PCAL6416A** on the I²C bus; its defining feature for X65 is **interrupt-mask registers**, so software only ever sees IRQs for the input transitions it has explicitly requested.

The graphics chip (CGIA) is wired to **NMI**, where it raises raster-line and vertical-blank interrupts (configured per display-list entry — see [Chapter 4: Graphics](4_graphics.md)). Because NMI bypasses the IRQ mask, raster effects remain responsive even while ordinary IRQ handling is disabled.

In-depth coverage of writing interrupt handlers in 65816 assembly is reserved for [Chapter 15: Advanced Topics](../2/15_advanced.md).

## System Clock and Performance

`PHI2` is generated in software by the RP2350 PIO rather than by a fixed crystal divider, which means the X65's headline clock is configurable and tracks whatever the firmware programs the PIO state machine to produce. The CPU throughput bottleneck is the memory round-trip latency, thus bumping the `PHI2` clock above certain thresholds yields diminishing returns — the CPU simply spends most time waiting for memory.

Combined with the VAB optimisation and the L2 cache described above — which together let the CPU skip the PSRAM round-trip on internal-only cycles *and* on cache hits — the X65 sustains noticeably more useful work per second than a hardwired-`PHI2` design at the same nominal clock.

### Clock Tree

The NORTH chip runs at **336 MHz** — a multiple of 48, chosen so that the USB clock can be derived cleanly from the system PLL. This frees the RP2350's second PLL to drive the HSTX (DVI) output, which must produce a pixel-perfect 60 Hz frame. If video timing were still divided from the system clock, overclocking NORTH would drag the framerate with it; decoupling the two means VBI-synchronous software can rely on a clean 60 Hz regardless of CPU-side tuning.

PSRAM runs at a convenient integer division of the same 336 MHz — **112 MHz** — which happens to be the sweet spot for the APS6404L-3SQR-ZR parts at 3.3 V.

A quick status dump from the monitor shows the whole tree:

```text
]status
X65 microcomputer

CPU : ~4.2MHz
CORE: 336.0MHz
DVI : 768x480@60.0Hz/24bpp
RAM : 16MB, 2 banks, 112.0MHz
```

The `CPU` line is the effective `PHI2` the 65C816 is clocked at — an estimation based on memory access speed and cache performance.
