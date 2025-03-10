# Chapter 11: Graphics Programming

```{note}
This chapter is still under construction.
```

## Interfacing with CGIA

---

## Display List

The **Color Graphic Interface Adaptor (CGIA)** of the X65 microcomputer utilizes a **display list system** for defining and controlling video output. This approach is inspired by **Atari's ANTIC processor**, with additional flexibility to support **text, bitmap, multicolor, and advanced graphical modes**.

### Overview of the CGIA Display List

A **display list** is a series of instructions stored in memory that defines how the CGIA renders each scanline. Instead of the CPU handling screen rendering directly, the **Video Processing Unit (VPU)** interprets the display list, fetching **graphics data, colors, and character definitions** on a per-line basis. This method allows for a dynamic and efficient **beam-chased** video output without CPU intervention.

### Display List Instructions

The CGIA display list consists of **two main categories of instructions**:

1. **Control Instructions (0-7)** – Used for line fills, jumps, memory loads, and register manipulation.
2. **Mode Row Instructions (8-F)** – Define how the CGIA should render specific scanlines.

For instructions requiring additional data, these follow the instruction code.

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
- **4** – Load an 8-bit value into a register. Each plane has 16 multipurpose registers, indexed 0 - 15.
  - Bits 7-4: Register index.
- **5** – Load a 16-bit value into a register. Plane registers are treated line 8 x 16-bit values, indexed 0 - 7.
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

## Plane Registers Interpretation

Each **CGIA plane** has **16 associated registers**, but their interpretation depends on the selected **plane type** and **graphics mode**. The last **display list mode instruction** determines how the plane registers are utilized. The register structure varies based on whether the plane is handling **background graphics, HAM mode, affine transformations, or sprites**.

### Background Plane Registers

- **flags** _(8-bit)_ – Configuration flags.
- **border_columns** _(8-bit)_ – Number of border columns.
- **row_height** _(8-bit, unsigned)_ – Height of each row.
- **stride** _(8-bit, unsigned)_ – Memory stride for addressing screen data.
- **scroll_x, scroll_y** _(8-bit, signed)_ – Fine scrolling values.
- **offset_x, offset_y** _(8-bit, signed)_ – Base offsets for plane positioning.
- **shared_color** ×2 _(8-bit each)_ – Two shared colors.

#### Background Plane Flags

- **Bit 0** – Color 0 is transparent.
- **Bits 1-3** – Reserved.
- **Bit 3** – Border is transparent.
- **Bit 4** – Double-width pixel mode.
- **Bits 5-7** – Reserved.

```csv
PLANE_MASK_TRANSPARENT        %00000001
PLANE_MASK_BORDER_TRANSPARENT %00001000
PLANE_MASK_DOUBLE_WIDTH       %00010000
```

### HAM Mode Registers

- **flags, border_columns, row_height** _(8-bit)_ – Same as background mode.
- **base_color** ×8 _(8-bit each)_ – The **8 base colors** used for HAM modifications.

### Affine Transform Mode Registers

- **flags, border_columns, row_height** _(8-bit)_ – Same as before.
- **texture_bits** _(8-bit)_ – Defines **texture width and height** in bits (size is power of two).
- **u, v** _(16-bit, signed)_ – Texture coordinates for the upper-left corner.
- **du, dv** _(16-bit, signed)_ – Texture step increments per pixel.
- **dx, dy** _(16-bit, signed)_ – Transform coefficients for scaling and rotation.

### Sprite Plane Type Registers

- **active** _(8-bit)_ – Bitmask indicating which sprites are active.
- **border_columns** _(8-bit, unsigned)_ – Number of border columns.
- **start_y, stop_y** _(8-bit, unsigned)_ – Vertical range where sprites are visible.

By utilizing **dynamic plane register interpretation**, the CGIA enables **highly versatile graphics rendering**, allowing the X65 to seamlessly mix **background layers, sprites, advanced color manipulation, and affine transformations** in real time.

---

## Implementing HAM Mode Graphics

### Hold-And-Modify (HAM) Mode

HAM mode is a **unique graphical technique** that allows for an **expanded color range** by modifying the color of each pixel based on the preceding pixel. This technique, inspired by the **Amiga HAM mode**, enables an expanded color range without requiring a full 8-bit per-pixel framebuffer.

### HAM Encoding Format

HAM mode operates using **6-bit commands**, where **4 screen pixels are packed into 3 bytes**:

```
[CCCDDD] - C: Command bits, D: Data bits
```

#### Commands

- **000** – Load a base color from an 8-color palette (DDD specifies which color).
- **001** – Blend the current pixel with another color from the 8-color palette.
- **CCS** – Modify a single color channel:
  - **01S** – Modify the **Red** channel.
  - **10S** – Modify the **Green** channel.
  - **11S** – Modify the **Blue** channel.
  - **S** – Sign bit (0 = positive delta, 1 = negative delta).
  - **DDD** – Delta value (000 represents +1).

### Rendering HAM Mode

The **CGIA processes HAM mode** by starting with a **base color** and applying **per-pixel modifications**. This means that each pixel is calculated based on the **previous pixel**, allowing for smooth color transitions across the screen. However, this also means that **artifacts may appear** when picture colors are changing rapidly.

### Optimizing HAM Graphics

Due to its **serial dependency**, HAM mode benefits from **careful palette selection** and **strategic placement of base color indices** to minimize unwanted color blending. Developers can improve visual quality by:

- **Careful base colors selection** to minimize artifacting.
- **Using blending instructions** to smoothly transition between colors.
- **Leveraging display list registry set instructions and/or interrupts (DLI)** to alter base-colors mid-frame.

### Applications of HAM Mode

HAM mode is particularly useful for **high-color images, gradients, and advanced shading effects**. Unlike traditional indexed-color modes, HAM can **generate a broader range of colors** while keeping memory usage low.

By integrating **HAM Mode (MODE6) into the X65 graphics pipeline**, developers can achieve **highly detailed visuals** with **minimal additional memory overhead**, making it a powerful tool for static images.

## Creating Mixed-Mode Display Lists
