# Appendix C: From 6502 to 65816 Migration Guide

## Introduction: Bridging the 8-bit and 16-bit Worlds

For many programmers familiar with the classic 6502 architecture, the transition to the 65816 presents both exciting opportunities and potential challenges. This chapter serves as a comprehensive guide to help experienced 6502 programmers leverage their existing knowledge while adapting to the enhanced capabilities of the 65816 CPU in the X65 microcomputer.

## Register Enhancements

### Accumulator (A)

| 6502                                           | 65816                                               |
| ---------------------------------------------- | --------------------------------------------------- |
| 8-bit only                                     | Can be toggled between 8-bit and 16-bit modes       |
| Limited arithmetic range                       | Extended range in 16-bit mode                       |
| Operations often require multiple instructions | Many operations can be done in a single instruction |

**Code Example: 16-bit Addition**

```assembly
; 6502: Adding 16-bit values
LDA low_byte1    ; Load low byte
CLC             ; Clear carry
ADC low_byte2    ; Add low bytes
STA result_low   ; Store low byte result
LDA high_byte1   ; Load high byte
ADC high_byte2   ; Add high bytes with carry
STA result_high  ; Store high byte result

; 65816: Adding 16-bit values (accumulator in 16-bit mode)
REP #$20        ; Set 16-bit accumulator mode
LDA value1      ; Load 16-bit value
CLC             ; Clear carry
ADC value2      ; Add 16-bit values
STA result      ; Store 16-bit result
SEP #$20        ; Return to 8-bit accumulator mode if needed
```

### Index Registers (X, Y)

| 6502                                     | 65816                                         |
| ---------------------------------------- | --------------------------------------------- |
| 8-bit only                               | Can be toggled between 8-bit and 16-bit modes |
| 256-byte indexing without crossing pages | 16-bit indexing capabilities                  |
| Page-crossing penalties                  | More flexible memory addressing               |

**Code Example: Copying Memory Blocks**

```assembly
; 6502: Copy 256 bytes (limited by 8-bit Y register)
LDY #0          ; Initialize counter
.loop
LDA source,Y    ; Load from source
STA dest,Y      ; Store to destination
INY             ; Increment counter
BNE .loop       ; Loop until Y wraps around to 0

; 65816: Copy larger blocks with 16-bit index
REP #$10        ; Set 16-bit index registers
LDY #0          ; Initialize counter
.loop
LDA source,Y    ; Load from source
STA dest,Y      ; Store to destination
INY             ; Increment counter
CPY #1024       ; Compare with larger value
BNE .loop       ; Loop until Y reaches 1024
SEP #$10        ; Return to 8-bit index mode if needed
```

### Direct Page Register (D)

The Direct Page register is a new concept for 6502 programmers, replacing the fixed Zero Page with a movable 256-byte region that can be positioned anywhere in bank 0.

| 6502 Zero Page                        | 65816 Direct Page                                        |
| ------------------------------------- | -------------------------------------------------------- |
| Fixed at $0000-$00FF                  | Can be positioned anywhere in bank 0                     |
| Always accessed with one-byte address | Accessed with one-byte address relative to D register    |
| Shared among all processes            | Can allocate unique direct pages for different processes |

**Code Example: Setting up and Using Direct Page**

```assembly
; 65816: Setting up a custom Direct Page
LDA #$2000      ; Low byte of desired Direct Page address
TCD             ; Transfer to Direct Page register (D)

; Using Direct Page addressing (similar to Zero Page)
LDA $10         ; Accesses $2010 (D + $10)
STA $20         ; Stores to $2020 (D + $20)
```

## Memory Management Differences

### 24-bit Addressing

While the 6502 was limited to 64KB of addressable memory, the 65816 can access up to 16MB using 24-bit addresses. The X65 takes full advantage of this with its flat memory model.

**Code Example: Accessing Memory Beyond 64KB**

```assembly
; 65816: Accessing memory in different banks
SEP #$20        ; 8-bit accumulator mode
LDA #$01        ; Load bank number
PHA             ; Push to stack
PLB             ; Pull to Data Bank register
LDA $8000       ; Access $018000
```

### Long Addressing

The 65816 introduces long addressing modes that explicitly specify all 24 bits of an address.

**Code Example: Long Addressing**

```assembly
; 65816: Long addressing example
LDA $01:8000    ; Load from absolute address $018000
JSL $02:4000    ; Jump to subroutine at $024000
```

## New Instructions and Capabilities

### Block Move Operations

The 65816 adds powerful block move instructions that can efficiently copy or move data between memory locations.

**Code Example: Block Move**

```assembly
; 65816: Block move with MVP (move positive)
REP #$10        ; 16-bit index registers
LDX #$0000      ; Source offset
LDY #$0000      ; Destination offset
LDA #$00FF      ; Number of bytes to move minus 1
MVN #$01,#$02   ; Move $100 bytes from bank $01 to bank $02
```

### Stack Operations

The 65816 introduces stack-relative addressing modes, making it easier to implement local variables and pass parameters.

**Code Example: Stack-Relative Addressing**

```assembly
; 65816: Using stack for local variables
TSC             ; Transfer stack pointer to C
TCD             ; Use stack as direct page
LDA 1,S         ; First parameter (1 byte offset from stack)
LDA 3,S         ; Second parameter (3 bytes offset from stack)
```

## Practical Migration Strategy

1. **Start in 8-bit mode**: Begin by using the 65816 in a way that's similar to the 6502, then gradually incorporate 16-bit features.

2. **Migrate core routines first**: Identify performance-critical routines and optimize them with 16-bit operations.

3. **Use bank zero efficiently**: The first 64KB (bank zero) has special significance for many instructions, so plan your memory layout accordingly.

4. **Document register size changes**: Use comments to mark where you change register sizes to avoid confusion.

5. **Leverage new addressing modes**: Take advantage of stack-relative, direct-page indirect, and long addressing modes for cleaner, more efficient code.

## Common Pitfalls

1. **Forgetting register sizes**: Always track whether your registers are in 8-bit or 16-bit mode.

2. **Bank boundary issues**: Remember that some instructions don't cross bank boundaries unless using long addressing modes.

3. **Stack management complexity**: The stack is more flexible but requires careful management, especially with subroutines.

4. **Misaligned direct page**: Setting the direct page to an odd address can cause performance penalties.

## Summary

Moving from the 6502 to the 65816 opens up significant new capabilities while building on familiar concepts. By understanding the key differences and leveraging the enhanced features of the 65816, experienced 6502 programmers can quickly become productive with the X65 microcomputer's architecture, taking advantage of its expanded memory, improved performance, and enhanced instruction set.
