# Appendix E: X65 Quick Reference Cheat Sheet

## 65816 CPU Registers

| Register | Size     | Description           | Notes                                     |
| -------- | -------- | --------------------- | ----------------------------------------- |
| A (,B=C) | 8/16-bit | Accumulator           | Width controlled by m flag (0=16-bit)     |
| X        | 8/16-bit | X Index Register      | Width controlled by x flag (0=16-bit)     |
| Y        | 8/16-bit | Y Index Register      | Width controlled by x flag (0=16-bit)     |
| S        | 16-bit   | Stack Pointer         | Points to next free location on stack     |
| D        | 16-bit   | Direct Page Register  | Base address for direct page addressing   |
| PBR      | 8-bit    | Program Bank Register | Current code bank (upper 8 bits of PC)    |
| DBR      | 8-bit    | Data Bank Register    | Default data bank for absolute addressing |
| P        | 8-bit    | Processor Status      | Contains status flags                     |

## Status Register (P) Flags

| Bit | Flag | Description               | When Set (1)            | When Clear (0)         |
| --- | ---- | ------------------------- | ----------------------- | ---------------------- |
| 7   | N    | Negative                  | Result is negative      | Result is positive     |
| 6   | V    | Overflow                  | Overflow occurred       | No overflow            |
| 5   | M    | Memory/Accumulator Select | 8-bit accumulator       | 16-bit accumulator     |
| 4   | X    | Index Register Select     | 8-bit index registers   | 16-bit index registers |
| 3   | D    | Decimal Mode              | BCD arithmetic mode     | Binary arithmetic mode |
| 2   | I    | IRQ Disable               | IRQ interrupts disabled | IRQ interrupts enabled |
| 1   | Z    | Zero                      | Result is zero          | Result is non-zero     |
| 0   | C    | Carry                     | Carry occurred          | No carry occurred      |

## Common 65816 Instructions

### Register Instructions

```assembly
REP #$30      ; Reset bits in P (set 16-bit A,X,Y mode)
SEP #$30      ; Set bits in P (set 8-bit A,X,Y mode)
XBA           ; Exchange B and A (swap high/low bytes)
TCS           ; Transfer C (16-bit A) to S
TCD           ; Transfer C (16-bit A) to D
PHB/PLB       ; Push/Pull Data Bank Register
PHD/PLD       ; Push/Pull Direct Page Register
PHK           ; Push Program Bank Register
```

### Mode Switching

```assembly
CLC           ; Clear carry
XCE           ; Exchange Carry with Emulation flag (enter native mode)
SEC           ; Set carry
XCE           ; Exchange Carry with Emulation flag (enter emulation mode)
```

### Bank Register Management

```assembly
LDA #$01      ; Load bank number
PHA           ; Push to stack
PLB           ; Pull to Data Bank Register
```

### Memory Block Operations

```assembly
MVN src,dst   ; Move memory block (ascending)
MVP src,dst   ; Move memory block (descending)
```

### 16-bit Operations

```assembly
REP #$20      ; 16-bit accumulator mode
LDA #$1234    ; Load 16-bit immediate value
ADC #$4321    ; 16-bit addition
SBC #$0001    ; 16-bit subtraction
```

### Long Addressing

```assembly
JML $123456   ; Jump Long (sets Program Bank Register)
JSL $123456   ; Jump to Subroutine Long
RTL           ; Return from Subroutine Long
LDA $123456   ; Load from long address
STA $123456   ; Store to long address
```

## Common Assembly Code Snippets

### System Initialization

```assembly
    CLC                     ; Clear carry flag
    XCE                     ; Switch to native mode
    REP #$30                ; 16-bit A,X,Y
    LDA #$0000              ; Set direct page to 0
    TCD
    LDX #$01FF              ; Set up stack at $01FF
    TXS
    SEP #$20                ; 8-bit A for I/O operations
```

### Playing a Sound Effect

```assembly
    ; Set frequency for PWM channel 0
    LDA #<440               ; A note (440 Hz) - low byte
    STA $FF20
    LDA #>440               ; A note (440 Hz) - high byte
    STA $FF21

    ; Set duty cycle (50%)
    LDA #$7F
    STA $FF22
```
