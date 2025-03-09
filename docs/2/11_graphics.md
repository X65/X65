# Chapter 11: Graphics Programming

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

---

## Implementing HAM Mode Graphics

Hold-and-Modify (HAM) mode is a **unique graphical technique** that allows for an **expanded color range** by modifying the color of each pixel based on the preceding pixel. The X65 implements **HAM mode as MODE6**, where pixel data is stored in a **compressed 6-bit format** that allows for flexible **per-pixel color manipulation**.

### HAM Mode Encoding

Each **HAM command** is 6 bits long, with **four pixels packed into three bytes**. The encoding is structured as follows:

```
[CCCDDD] - C (Command bits), D (Data bits)
```

- **000** – Load a **base color index** (one of 8 pre-defined colors).
- **001** – Blend the current pixel color with one of the 8 base colors.
- **CCS (01, 10, 11)** – Modify an individual color channel:
  - **01S** – Modify **Red**
  - **10S** – Modify **Green**
  - **11S** – Modify **Blue**
  - **S** – Sign bit (0 = increase, 1 = decrease)
  - **DDD** – Delta (adjustment magnitude, offset by 1 so `000` means +1)

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
