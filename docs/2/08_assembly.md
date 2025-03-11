# Chapter 8: Introduction to 65816 Assembly

## Overview of the 65C816 CPU

The **WDC 65C816** (commonly referred to as the 65816) is a versatile 16-bit microprocessor developed by the Western Design Center (WDC). Building upon the foundation of the 8-bit 6502 and its CMOS variant, the 65C02, the 65816 introduces several enhancements that make it suitable for more advanced computing applications.

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

### Enhanced Register Set

The 65816 expands upon the register set of the 6502, with significant enhancements to improve programming flexibility:

#### Accumulator (A)

- **Width Toggle:** Can operate as either 8-bit or 16-bit by toggling the "m" (memory/accumulator select) bit in the Processor Status (P) register
- **16-bit Mode:** When operating in 16-bit mode, the accumulator is represented as "C" (16-bit combined accumulator)
- **8-bit Mode:** In 8-bit mode, the accumulator is divided into "A" (low byte) and "B" (high byte, which becomes accessible via XBA instruction)

#### Index Registers (X and Y)

- **Width Toggle:** Like the accumulator, can operate as either 8-bit or 16-bit by toggling the "x" (index register select) bit in the P register
- **Scaling:** In 16-bit mode, these registers can index larger blocks of memory, making table operations more efficient

#### Direct Page Register (D)

- **16-bit Value:** Replaces the concept of "Zero Page" from the 6502
- **Flexible Positioning:** Can be set to any location within the first 64KB of memory
- **No Page Alignment Required:** Unlike traditional zero page operations, the direct page does not need to be aligned to a page boundary
- **Dynamic Relocation:** Enables multiple processes to have their own "zero page" equivalent, facilitating multitasking

#### Stack Pointer (S)

- **16-bit Width:** Expanded from 8-bit to 16-bit in native mode
- **Relocatable Stack:** The stack can be positioned anywhere in bank 0
- **Enhanced Stack Operations:** New stack-relative addressing modes enable more efficient subroutine and function implementations

#### Program Bank Register (PBR)

- **8-bit Value:** Determines the current 64KB bank for program execution
- **Jump Operations:** Automatically updated during long jumps and calls, and saved during interrupts
- **Return Operations:** Restored during RTS and RTL instructions

#### Data Bank Register (DBR)

- **8-bit Value:** Specifies the current 64KB bank for data operations
- **Absolute Addressing:** Used with absolute addressing modes to form the complete 24-bit address

#### Processor Status Register (P)

- **New Status Flags:** Includes all 6502 flags plus two new flags for register width control
- **m Flag:** Controls the width of the accumulator (0 = 16-bit, 1 = 8-bit)
- **x Flag:** Controls the width of index registers (0 = 16-bit, 1 = 8-bit)
- **e Flag:** Determines emulation mode (0 = native mode, 1 = emulation mode)

### Expanded Addressing Modes

The 65816 significantly extends the addressing modes available to programmers:

#### 6502-Compatible Modes

- **Immediate:** Data is provided in the instruction
- **Absolute:** Full address is specified in the instruction
- **Zero Page:** Address in page 0 (becomes Direct Page in 65816)
- **Indexed:** Address is modified by X or Y registers
- **Indirect:** Address points to location containing the final address
- **Pre-Indexed Indirect & Post-Indexed Indirect:** Combinations of indirect and indexed addressing

#### New 65816-Specific Modes

- **Direct Page:** Replaces Zero Page, but offset by D register
- **Stack Relative:** Addressing relative to the stack pointer
- **Stack Relative Indirect Indexed:** Combining stack relative with Y-indexing
- **Block Move:** Special mode for efficiently copying memory blocks
- **Program Counter Relative Long:** Extended branching capability (16-bit offset)
- **Absolute Long:** Full 24-bit addressing across the entire memory space
- **Absolute Long Indexed:** 24-bit addressing with index register offset
- **Absolute Indexed Indirect:** Address calculated from base and X register, then used as pointer

### Practical Usage of Registers

#### Accumulator Management

```assembly
; Toggle accumulator size
REP #$20      ; Reset m bit - 16-bit accumulator mode
SEP #$20      ; Set m bit - 8-bit accumulator mode

; 16-bit accumulator operations
REP #$20
LDA #$1234    ; Load 16-bit value into accumulator
STA $2000     ; Store 16-bit value to memory

; Access 8-bit components in 16-bit mode
XBA           ; Exchange B and A accumulators
```

#### Index Register Techniques

```assembly
; Toggle index register size
REP #$10      ; Reset x bit - 16-bit index mode
SEP #$10      ; Set x bit - 8-bit index mode

; 16-bit indexing
REP #$10
LDX #$0000    ; Start at beginning of table
LOOP
    LDA $2000,X  ; Load from table
    STA $4000,X  ; Store to destination
    INX #2       ; Increment by 2 bytes
    CPX #$0100   ; Check if reached end (256 bytes)
    BNE LOOP     ; Continue if not at end
```

#### Direct Page Utilization

```assembly
; Set up direct page for efficient access
LDA #$1000
TCD           ; Transfer 16-bit A to direct page register

; Use direct page addressing (faster and smaller code)
LDA $10       ; Access memory at $1010
STA $20       ; Store to memory at $1020
```

#### Bank Register Management

```assembly
; Set data bank register
LDA #$01      ; Bank 1
PHA           ; Push to stack
PLB           ; Pull to bank register

; Long addressing across banks
LDA $020000   ; Load from absolute address in bank 2
STA $010000   ; Store to absolute address in bank 1

; Return to previous bank
PHB           ; Push current bank
LDA #$02      ; Bank 2
PHA
PLB           ; Set to bank 2
; ... operations in bank 2 ...
PLB           ; Restore previous bank
```

## Transitioning from 6502 to 65816

### Key Differences for 6502 Programmers

The 65816 maintains backwards compatibility with the 6502 while introducing significant enhancements. When transitioning from 6502 to 65816 programming, consider these important differences:

#### Register Size Management

Unlike the 6502, the 65816 requires explicit management of register sizes:

```assembly
; Standard startup sequence for 65816 native mode
CLC           ; Clear carry flag
XCE           ; Exchange carry with emulation flag (enter native mode)
REP #$30      ; Set 16-bit accumulator and index registers (clear m and x bits)
```

#### Memory Width Considerations

When operating with mixed register widths, be mindful of memory access patterns:

```assembly
; Mixed width operations
REP #$20      ; 16-bit accumulator
SEP #$10      ; 8-bit index registers

LDA #$1234    ; 16-bit immediate
STA $2000     ; 16-bit store

LDX #$10      ; 8-bit immediate
LDA $00,X     ; 16-bit load from direct page + X
```

#### Stack Usage Differences

The 65816's 16-bit stack pointer enables more sophisticated stack operations:

```assembly
; Save current frame pointer
PHD           ; Push direct page register
TCS           ; Transfer current stack pointer to C (16-bit A)
SEC
SBC #$20      ; Reserve 32 bytes of local variables
TCD           ; Set new direct page (frame pointer)

; ... function body ...

; Restore previous frame
LDA 1,S       ; Load saved direct page
TCD           ; Restore previous direct page
PLA           ; Adjust stack pointer (discard saved DP)
```

#### Direct Page vs. Zero Page

While zero page on the 6502 is fixed at memory locations $0000-$00FF, the 65816's direct page can be relocated:

```assembly
; Set up direct page for a specific routine
LDA #$2000
TCD           ; Direct page now at $2000

; Access variables at direct page offsets
LDA $10       ; Actually accesses $2010
STA $20       ; Actually accesses $2020

; Restore system direct page when done
LDA #$0000
TCD
```

#### Handling 24-bit Addressing

The 65816's full address space requires 24-bit addressing for cross-bank access:

```assembly
; Jump to routine in another bank
JML ROUTINE   ; Long jump (sets Program Bank Register)

; Access data in another bank
LDA $01:2000  ; Explicit bank notation (bank 1, offset $2000)
LDA $012000   ; 24-bit absolute address

; Return from cross-bank call
RTL           ; Return from long call
```

### Common Pitfalls When Transitioning

#### Register Width Confusion

One of the most common errors is forgetting the current register width state:

```assembly
; Problematic code
REP #$20      ; 16-bit accumulator
; ... other code ...
LDA #$FF      ; Loads $00FF, not just $FF!

; Better practice
REP #$20      ; 16-bit accumulator
; ... operations requiring 16-bit ...
SEP #$20      ; Explicitly return to 8-bit when done
LDA #$FF      ; Loads $FF as expected
```

#### Stack Pointer Assumptions

The expanded stack requires careful management:

```assembly
; 6502 approach (problematic on 65816)
LDX #$FF
TXS           ; Only sets low byte of S in native mode!

; Correct 65816 approach
REP #$30      ; 16-bit registers
LDX #$01FF    ; Full 16-bit stack pointer value
TXS           ; Set stack pointer correctly
```

#### Bank Boundary Crossing

Crossing bank boundaries requires explicit handling:

```assembly
; Problem: This may not work if the table crosses a bank boundary
LDX #$FFFC
LDA $2000,X   ; Might attempt to read across bank boundary

; Solution: Use long addressing
LDA $01:2000,X  ; Explicitly specify bank
```

#### Direct Page Alignment Penalties

Non-aligned direct page addresses incur a performance penalty:

```assembly
; Inefficient (cycle penalty)
LDA #$1001    ; Not page-aligned
TCD

; Efficient
LDA #$1000    ; Page-aligned
TCD
```

### Leveraging 65816 Advantages

#### Block Move Instructions

The 65816 introduces powerful block move instructions:

```assembly
; Move up to 64KB between banks
REP #$30      ; 16-bit registers
LDX #$0000    ; Source offset
LDY #$0000    ; Destination offset
LDA #$1000    ; Number of bytes to move (4KB)
MVN #$01,#$02 ; Move ascending from bank 1 to bank 2
```

#### 16-bit Arithmetic

Take advantage of 16-bit operations for more efficient math:

```assembly
; 16-bit addition and subtraction
REP #$20
LDA #$1234
ADC #$2000    ; 16-bit addition
SBC #$0100    ; 16-bit subtraction

; 16-bit comparisons
CMP #$FFFF    ; Compare with 16-bit value
```

#### Stack Frames for Subroutines

Implement more sophisticated function call patterns:

```assembly
; Function with stack frame
MyFunction:
    PHD           ; Save caller's direct page
    TCS           ; Get current stack pointer
    SEC
    SBC #$10      ; Reserve 16 bytes local storage
    TCD           ; D = new frame pointer

    ; Access parameters relative to frame
    LDA $13,S     ; First parameter (3 bytes past saved DP)

    ; Access local variables
    STA $01       ; Local variable at DP+1

    ; Return
    PLD           ; Restore caller's direct page
    RTL           ; Return from subroutine
```

By understanding these key differences and leveraging the 65816's enhanced capabilities, programmers can transition smoothly from 6502 to 65816 development and take full advantage of the X65 microcomputer's architecture.
