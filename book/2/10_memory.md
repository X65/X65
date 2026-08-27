# Chapter 10: Memory Management

```{note}
This chapter is still under construction.
```

The hardware memory model is covered in [Chapter 3](../1/3_memory_storage.md). This chapter is the programming-side counterpart: how to reach, arrange, and size memory from 65C816 assembly, and the specific idioms that matter for the two execution contexts the X65 supports — **bare-metal** and **under OS/816**.

## Reaching PSRAM

The 65C816's addressing modes give three natural ways to touch PSRAM:

- **Absolute long** (`lda $123456`) — any byte in the 16 MB range, no setup needed. Useful for one-off loads / stores across bank boundaries.
- **DBR-relative absolute** (`lda $1234`) — 16-bit operand inside the current Data Bank. A two-byte operand instead of three, but only works inside whichever bank `DBR` points at.
- **Direct-page** (`lda <foo`) — 8-bit operand inside the current Direct Page. Shortest and fastest of the three; this is where hot variables live.

### Bank Crossing

The PSRAM is physically two 8 MB banks, but NORTH stitches them into a flat address space so bank crossing at `$800000` is transparent to the CPU — no special instruction is needed to walk from one bank into the other.

The one thing the CPU itself does care about is **bank-wrap on 16-bit addressing**: a 16-bit `lda $FFFF` near the top of a bank does **not** spill into the next bank. The 65C816 wraps within the current bank. Use `long` addressing or set `DBR` first if you need to straddle.

### DBR and PBR Management

Two common patterns:

```asm
; Point DBR at the same bank as code (needed for tables reachable via absolute)
    phk
    plb

; Temporarily switch DBR to another bank for a data-heavy routine
    lda #$01                ; target bank
    pha
    plb
    ; ... do work with $BBBBBB-style absolute loads ...
    phk
    plb                     ; restore
```

`PBR` (program bank) changes only on long branches (`jml`, `jsl`) and on return (`rtl`). A routine that wants to live outside bank 0 simply has to be reached via `jsl` / `rtl`; the assembler and linker will emit the correct operand widths.

## Direct Page

Direct page is 256 bytes, reachable through the `D` register. The idiomatic setup at program start:

```asm
    rep #$20                ; 16-bit A
    lda #$0000              ; or wherever this program wants DP
    tcd                     ; TCD: transfer 16-bit C (A+B) into D
```

Now `lda <foo` uses one-byte operands against `D + <foo>`. Direct-page addressing is the fastest addressing mode the 65C816 has.

### Per-routine / per-task direct pages

A single direct page is fine for a short program. For larger systems, move `D` at routine or task boundaries:

```asm
    ; Save caller's DP, install our own
    phd
    pea   our_dp_base
    pld
    ; ... routine body uses its own direct page ...
    pld                     ; restore caller's
```

This is the same idiom a language runtime or an RTOS would use for per-thread locals — cheap, one-instruction sites per window, no heap.

### Under OS/816

Each OS/816 task gets a dedicated 64 KB memory bank for its code and data, plus a 256-byte page in bank 0 shared between the task's direct page (at the bottom, growing up) and its hardware stack (at the top, growing down). Before launching, the OS has already set `D` to the task's assigned direct-page base inside that bank-0 page. Applications therefore typically do **not** call `TCD` themselves — doing so would steer the direct page away from the page the stack shares and break the co-location. If a routine really needs a different direct page (e.g., to reach a library's per-module statics), **save and restore `D` around the call** using the `PHD` / `PLD` pattern above.

## Hardware Stack

In native mode `S` is 16 bits wide, so stack instructions see a full 64 KB on bare metal:

```asm
    clc
    xce                     ; native mode
    rep #$10                ; 16-bit X
    ldx #$01FF              ; stack base
    txs
```

### 16-bit pushes and pulls

With `M` clear, `PHA` / `PLA` move 16 bits; with `X` clear, `PHX` / `PHY` / `PLX` / `PLY` do the same on the index registers. Typical stack-frame prologue for a routine that takes a 16-bit argument and returns a 16-bit result:

```asm
    phd                     ; save caller's DP
    pha                     ; reserve 2 bytes for local
    tsc
    tcd                     ; point DP at the top of the stack frame
    ; ... locals at <0, args at <2, return at <4 ...
    pla                     ; drop local
    pld                     ; restore caller's DP
    rts
```

`TSC` / `TCD` combined is how the 65C816 builds stack frames cheaply: the direct-page base follows the stack pointer for the duration of the routine.

### Stack sizing under OS/816

The 256-byte DP-plus-stack page limits the **combined** direct-page + stack footprint to a little under 256 bytes. In practice, a well-behaved task keeps its direct page under ~64 bytes and its stack under ~192 bytes. Note that this limit applies only to the co-located DP + stack region — code, data, and any heavy working buffers live in the task's dedicated 64 KB bank and are not affected. If a routine really does need a deeper stack than the page can spare, the usual answer is to restructure the computation rather than switch stacks.

## XSTACK — Fastcall Argument Channel

The RIA exposes a **512-byte XSTACK** buffer inside NORTH, reached through the fastcall API at `$FFF0–$FFF3`. Its purpose is to carry call arguments and return values into and out of the NORTH chip without using the 65C816's hardware stack (which would risk a task switch under OS/816) and without passing pointers into PSRAM.

A typical fastcall sequence:

```asm
; Push N argument bytes onto XSTACK via the data port
    lda arg_byte_0
    sta $FFF2                ; XSTACK_DATA
    lda arg_byte_1
    sta $FFF2
    ; ... continue for N bytes ...

; Invoke syscall
    lda #SYSCALL_ID
    sta $FFF1                ; XSTACK_OP

; On return: result (if any) is left on XSTACK or in A
    lda $FFF2                ; XSTACK_DATA (pops one byte)
```

Exact register assignments for each fastcall live with the RIA API documentation; the book-level point is that XSTACK is a second, RIA-local stack used only for the fastcall API — do not use it as a general-purpose scratch area, and do not leave data on it across unrelated fastcalls.

## Summary

Memory on the X65 is easy on paper (16 MB flat, no bank switching, no ROM overlays) and mostly easy in practice: keep hot variables in direct page, use `PHD` / `PLD` around routines that touch it, and use absolute-long addressing when you genuinely need to straddle banks. Under OS/816, a task has a dedicated 64 KB bank for code and data plus a 256-byte DP-plus-stack page in bank 0 — the latter is the one tight constraint; everything else is "normal 65C816". XSTACK is the fastcall argument channel and nothing else — do not repurpose it.
