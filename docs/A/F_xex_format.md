# Appendix F: XEX File Format

Every X65 binary is a `.xex` file — boot ROMs, user applications, monitor `load` payloads. The format is derived from the Atari 8-bit `.xex` executable format, with three X65-specific extensions for the wider 24-bit address space and the firmware monitor's INFO command. This appendix documents what is on disk, byte for byte. The build pipeline that produces these files is covered in [Chapter 9](../2/09_dev_env.md); the OS/816 shell that loads user-application `.xex` files is in [Chapter 14](../2/14_os.md).

## Overall Structure

A `.xex` is a sequence of independently-addressed **segments**. Each segment names a memory range and supplies the bytes to load there. Segments are processed strictly in file order, and a later segment may overwrite bytes written by an earlier one. All multi-byte fields are little-endian (low byte first).

The file opens with bank `$00` selected as the implicit load target. Bank-switch sentinel segments (described below) change which bank subsequent segments target.

## File Header

The first two bytes of the file must be `$FF $FF`. The same `$FF $FF` magic may appear between later segments and is silently skipped by the loader; emitting it is optional after the first segment.

## Segment Layout

| Offset | Bytes | Field           | Notes                                             |
| ------ | ----- | --------------- | ------------------------------------------------- |
| 0–1    | 2     | Header          | `$FFFF` (required in 1st segment, optional after) |
| 2–3    | 2     | Start address A | Little-endian, `$0000`–`$FFFF`                    |
| 4–5    | 2     | End address B   | Little-endian, A ≤ B ≤ `$FFFF`                    |
| 6…     | B−A+1 | Payload         | Loaded into A…B of the current bank               |

After the payload, another segment may follow immediately (with or without a leading `$FFFF`) until end-of-file.

## Bank-Switch Sentinel — `$FFFE` Segment

The original Atari format addresses 64 KB; the X65 lives in a 24-bit space, so the loader needs a way to retarget banks mid-file.

A segment whose start address **and** end address are both `$FFFE` is **not** a one-byte write to address `$FFFE`. Instead, the loader reads the segment's single payload byte and uses it as the **bank number for all following segments**.

The bank starts at `$00` when the file is opened. A typical multi-bank `.xex` opens with bank-zero segments, then emits an `$FFFE`-sentinel with payload `$01` to switch banks, and the segments that follow now write into bank `$01`.

```text
FF FE   FF FE      ; start=$FFFE, end=$FFFE → sentinel
01                 ; bank for subsequent segments
```

## HELP/INFO Segment — `$FC00` in Bank `$00`

A segment whose start address is `$FC00` *and* whose target bank is `$00` is **not loaded into RAM**. The loader skips its payload entirely.

This region carries user-visible help/info text, displayed by the NORTH firmware monitor's `info` command (see [Chapter 6](../1/6_io.md)). The convention exists because `$FC00`–`$FCFF` is reserved for expansion-slot MMIO (see [Appendix A](A_memory_map.md)) — writing to it during load would be both pointless and unsafe — so the loader repurposes that address as a "this segment is metadata, skip it" signal.

In the words of the firmware help text:

> LOAD and INFO read ROM files from a USB drive. A ROM file contains both ASCII information for the user and binary information for the system. The ROM file is an Atari XEX-derived binary format, and may contain many chunks loaded into different memory areas. The chunk marked for load into the area starting at `$FC00` in bank `$00` is special: it carries HELP/INFO text and is **not** loaded into RAM. The `INFO` monitor command displays it.

A `$FFFE`-sentinel that has switched the current bank to a non-zero value **does not** cause subsequent `$FC00` segments to be skipped — only bank `$00` triggers the skip.

## Auto-Run via the Reset Vector — `$FFFC`–`$FFFD`

When the entire file has been processed, the loader checks whether bytes have been written to **both** `$FFFC` and `$FFFD` (the 65C816 reset vector low and high bytes, in any segment). If so, it releases the CPU reset; the CPU then begins execution at the address held in `$FFFC`–`$FFFD`.

The idiomatic auto-run footer is therefore a small segment writing the entry-point address into the reset vector:

```text
FF FC   FF FD      ; segment loading into $FFFC..$FFFD
00 20              ; reset vector = $2000
```

Files that omit the reset-vector load successfully but do not start running — a useful posture for libraries, overlays, and data-only `.xex` images.

## RUNAD / INITAD Compatibility Note

The classic Atari XEX vectors `RUNAD` (`$02E0`–`$02E1`) and `INITAD` (`$02E2`–`$02E3`) **are not honored by the X65 loader**. A `.xex` that relies on writing those addresses to start or hook execution will load its bytes successfully but will not run. Port such code by emitting an extra segment that writes the entry point to `$FFFC`–`$FFFD` instead.

## Worked Example

A minimal auto-running program — load `$42` into `A` at address `$2000`, then run from there:

```text
FF FF              ; magic (required, first segment only mandatory)
00 20   01 20      ; start=$2000, end=$2001
A9 42              ; payload: LDA #$42
FF FC   FF FD      ; reset-vector segment header
00 20              ; reset vector = $2000 → run after load
```

## Inspecting `.xex` Files in a Hex Editor

The **ImHex** hex editor (<https://imhex.werwolv.net>) ships an **XEX pattern** in its built-in Content Store that highlights segment headers, start/end addresses, and payload boundaries directly on the hex view. Install it via *Help → Content Store → search "XEX" → Install*; opening any `.xex` in ImHex afterwards turns the file into a colour-coded structural view, which is the fastest way to confirm that a build matches the layout described above. The pattern follows the Atari layout — the X65-specific `$FFFE` bank-switch sentinel and `$FC00`/bank-`$00` HELP/INFO segments appear as ordinary segments in the colouring; their meaning is implicit from their addresses.
