# Chapter 11: Graphics Programming

## Interfacing with CGIA

## Display List

The **Color Graphic Interface Adaptor (CGIA)** of the X65 microcomputer utilizes a **display list system** for defining and controlling video output. This approach is inspired by **Atari's ANTIC processor**, with additional flexibility to support **text, bitmap, multicolor, and advanced graphical modes**.

### Overview of the CGIA Display List

A **display list** is a series of instructions stored in memory that defines how the CGIA renders each scanline. Instead of the CPU handling screen rendering directly, the **Video Processing Unit (VPU)** interprets the display list, fetching **graphics data, colors, and character definitions** on a per-line basis. This method allows for a dynamic and efficient **beam-chased** video output without CPU intervention.

### Display List Instructions

The CGIA display list consists of **two main categories of instructions**:

1. **Control Instructions (0-7)** – Used for line fills, jumps, memory loads, and register manipulation.
2. **Mode Row Instructions (8-F)** – Define how the CGIA should render specific scanlines.

#### Control Instructions (0-7)

- **0** – Insert empty lines filled with the background color.
  - Bits 6-4: Number of empty lines.
  - Bit 7: Trigger a Display List Interrupt (DLI).
- **1** – Duplicate the last raster line multiple times.
- **2** – Jump to another location in the display list.
  - If DLI bit is set, execution waits for Vertical Blank before jumping.
- **3** – Load new memory pointers.
  - Bits 4-7 determine which offsets will be updated:
    - 4: Load Memory Scan (LMS) – Points to screen data.
    - 5: Load Foreground Scan (LFS) – Points to foreground color data.
    - 6: Load Background Scan (LBS) – Points to background color data.
    - 7: Load Character Generator (LCG) – Points to character shape definitions.
- **4** – Load an 8-bit value into a register.
- **5** – Load a 16-bit value into a register.
  - Bits 6-4: Register index.

#### Mode Row Instructions (8-F)

Mode row instructions define the type of graphics displayed on a scanline. The **lower three bits (0-2)** determine the mode, while **bit 3** differentiates them from control instructions.

- **8** – Reserved (TBD)
- **9** – Reserved (TBD)
- **A** – **Text/Tile Mode** – Uses character-based graphics with a foreground and background color map.
- **B** – **Bitmap Mode** – Direct pixel-based graphics.
- **C** – **Multicolor Text/Tile Mode** – Character-based graphics with **4-color cells**.
- **D** – **Multicolor Bitmap Mode** – Direct pixel graphics using **4-color cells**.
- **E** – **Hold-and-Modify (HAM) Mode** – Similar to **Amiga HAM**, where pixel colors can be modified based on previous pixels, enabling a larger color range.
- **F** – **Affine Transform Chunky Pixel Mode (MODE7)** – Inspired by **SNES MODE7**, allowing **rotation and scaling of graphics** for pseudo-3D effects.

Additionally, **bit 7** in any mode row instruction triggers a **Display List Interrupt (DLI)**, allowing for **scanline-specific effects, color changes, or sprite updates**.

### Using the Display List

A typical display list sequence begins with a **Load Memory Scan (LMS) instruction**, setting the screen’s base address. Subsequent instructions configure **row modes, colors, and additional graphics settings**. The final instruction often loops back to the beginning, ensuring continuous display refresh.

By leveraging the **CGIA’s display list system**, developers can efficiently mix **text, tiles, bitmaps, and advanced graphical effects** in a single frame without CPU overhead. This design enables **smooth scrolling, parallax effects, sprite multiplexing, and dynamic screen updates**, making the X65’s graphics system a powerful tool for retro-style computing and game development.

## Implementing HAM Mode Graphics

## Creating Mixed-Mode Display Lists
