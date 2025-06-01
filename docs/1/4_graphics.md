# Chapter 4: Graphics and Display

## The Color Graphics Interface Adaptor (CGIA)

The **CGIA (Color Graphics Interface Adaptor)** is the graphics subsystem of the X65 microcomputer, merging design elements from both the **Atari ANTIC** and **Commodore VIC-II/TED** architectures. It provides a **display list system** similar to ANTIC while incorporating a **separate character color mapping system** akin to the VIC-II and TED chips. This combination allows for a flexible yet efficient graphics pipeline, offloading much of the rendering workload from the CPU.

### Principles of Operation

The CGIA is an **on-bus device**, with **memory-mapped registers**. While the current implementation is done using a microcontroller firmware, that's only one approach - it could also be realized in an **FPGA** or **custom ASIC design**.

CGIA has **128 registers**, mapped at `$FF00..$FF7F`, used to configure its operation. It also includes **Direct Memory Access (DMA)** capabilities.

Its primary function is to manage and generate **four overlaying graphics planes**, each of which can be independently enabled or disabled. Each plane can operate in one of two modes:

* **Background graphics plane**
* **Sprites plane**

#### Background Graphics Plane Operation

When a plane is set to background graphics, it uses a **Display List** to control rendering on a **line-by-line basis**, and has up to **four memory scan pointers**:

* **memory\_data** (LMS)
* **foreground\_color** (LFS)
* **background\_color** (LBS)
* **character\_data** (LCG)

These are 16-bit pointers into memory, offset by a shared **background\_bank**, an 8-bit register that works similarly to the CPU’s `data_bank` and `program_bank` registers.

At the beginning of each display frame, CGIA loads the first display list instruction and begins rendering. It uses the instruction to determine one of 8 **display modes** (text, bitmap, multicolor, HAM, etc.) and draws **8 pixels at a time** using fetched character, bitmap, or color data. This process repeats to fill the raster line.

This is done **separately for each of the 4 planes**, which are **composited** in one line color buffer.

Once a raster line is fully constructed, it is pushed to a **hardware rasterizer** that converts the data into **TMDS symbols** for DVI/HDMI output. To achieve pixel doubling, each symbol is sent twice and each raster line is repeated once, producing an effective **384x240 resolution** on a **480p signal**.

#### Sprite Plane Operation

If a plane is configured as a **sprite plane**, it doesn't use a display list. Instead, it uses a **sprite descriptor table**, starting from a configurable pointer. At the beginning of the frame, **eight sprite descriptors** are loaded. Each sprite descriptor sets the sprite screen position, width, height and modes (multicolor, pixel doubling, etc.).

On each raster line, CGIA checks which sprites overlap that line (from `y_pos` to `y_pos + height`). It then fetches and draws the appropriate pixels into the raster line, based on the current `x_pos`. Sprites are drawn in **descending order**, so lower-numbered sprites overwrite higher-numbered ones (i.e., sprite 1 has higher priority than sprite 2).

Once the sprite finishes drawing (the raster line reaches its bottom), the sprite descriptor is reloaded with new contents. This allows the sprite to be reused in a lower part of the screen with a different X position and appearance, creating a built-in sprite multiplexer.

### Real-Time, Non-Persistent Raster Line Rendering

This rendering process occurs **in real time** for each of the **384 raster lines**, **60 times per second**. Importantly, there is **no persistent framebuffer**; every line is generated and transmitted immediately and is not stored in memory. A full framebuffer would require **270 KB**, which is impractical for the X65's architecture.

The different display list screen modes essentially serve as graphics **data compression formats**, enabling display of complex visuals using minimal memory. For example, a **single byte write can affect an 8x8 pixel matrix** in text mode. This allows the X65 to render rich graphics with efficient CPU and RAM usage.

### Display List and Scan Pointers

At the core of CGIA’s rendering system is the **display list**, a structured set of instructions that dictate how each scanline is rendered. Each display list **starts with a LOAD instruction**, specifying memory offsets for different scan pointer sources:

* **Memory Scan (LMS):** Defines the offset of display memory, holding character or tile data.
* **Foreground Color Scan (LFS):** Points to the foreground color map.
* **Background Color Scan (LBS):** Points to the background color map.
* **Character Generator (LCG):** Defines the shape of characters or tiles.

A display list can mix different graphical modes, much like ANTIC’s mode lines, allowing **mode switching per scanline**. The **row height of each mode** is configurable via the `row_height` plane register (1–256 raster lines per row). For example, a **C64-style text mode** uses a row height of 7, meaning 8 raster lines per character row.

Display list instructions also support **interrupt triggering**. A special flag in the instruction format enables an **interrupt to be generated at the end of the line**. This feature makes it possible to implement **precise mid-frame synchronization**, which is useful for effects like raster bars, scanline splits, color changes etc.

### Graphics Modes, Planes, and Color Palette

The CGIA graphics are rendered in **four overlaying planes**, each of which can be configured to display background graphics using a **display list** or sprite graphics using a **sprite descriptor table**. This allows for complex layering effects and efficient memory usage. The CGIA supports multiple graphics modes, which can be defined on a per-line basis through the display list system. These include:

* **Text and tile-based modes:** Character-based rendering similar to early home computers.
* **Bitmap modes:** Direct pixel-addressable graphics.
* **Multicolor modes:** Graphics use a **4-color-per-cell representation**, where each byte encodes four pixels. An optional **pixel-doubling flag** allows each logical pixel to cover 8 physical screen pixels, similar to C64 multicolor mode.

The X65 features a **256-color palette**, composed of 32 distinct hues, each available in 8 different brightness levels. Each mode can define **foreground and background colors per character/tile**, much like the VIC-II, but with additional flexibility due to CGIA’s three separate scan pointers.

### Beam-Chasing for Efficient Rendering

Once a display list is set up, **the CGIA handles beam-chasing automatically**, meaning that the CPU does not need to manually update graphics mid-frame. This approach ensures that the CPU remains free for **game logic, audio processing, or other computations**, while the **VPU (Video Processing Unit) executes display list instructions** and updates the screen accordingly.

### Raster Interrupts and Mid-Frame Changes

The CGIA also supports **raster interrupts**, which can be triggered at a vertical blank period or a specific scanline. This allows the CPU to perform **mid-frame updates** to registers or memory, enabling advanced effects or per-frame timing. Vertical Blank interrupt is most useful to trigger screen update code beetween frames.

### Sprite Multiplexing System

The CGIA features a **hardware sprite system** with built-in **multiplexing**. A sprite descriptor includes:

* **Position (X, Y) and size (Width, Height)**
* **Graphics data location**
* **Color attributes**
* **Next sprite descriptor offset** (controls automatic chaining for multiplexing)

When a sprite finishes rendering (i.e., it reaches `pos_y + lines_y` on the raster), **CGIA automatically loads the next sprite descriptor** from `next_dsc_offset`. This allows for a smooth, hardware-assisted sprite multiplexing system. If no further sprites should be loaded, the descriptor should point to itself (e.g., `0200 → 0200`).

### Sprite Dimensions and Features

A sprite in CGIA can be **8, 16, 24, 32, 40, 48, 56, or 64 pixels wide** and has a **variable height** defined by a 16-bit value (0–65535 pixels). This allows for highly customizable sprite sizes, ranging from small objects to large, detailed images.

### Additional Sprite Features

* **Multicolor mode:** Uses a **4-pixels-per-byte format**, meaning each sprite pixel takes 2 bits.
* **Double-width rendering:** Ensures multicolor sprites cover 8 pixels per byte (similar to C64).
* **Color index 0 transparency:** The first color is always transparent.

### Memory Banking for Graphics Rendering

The CGIA includes two configuration registers: **background\_bank** and **sprite\_bank**. These registers determine the high byte (bits 16–23) of the memory addresses used for rendering background and sprite graphics, respectively. This enables efficient graphics memory management, allowing both background and sprite data to reside in separate or shared memory banks.

### Advanced Graphics Modes

CGIA includes two special graphics modes that extend its capabilities beyond traditional tile and bitmap rendering:

* **HAM (Hold-And-Modify) Mode:** Inspired by the Amiga HAM mode, this allows modifying only part of the pixel color per scan, enabling more colors per screen at the cost of precision.
* **MODE7:** A chunky-pixel graphics mode with hardware-assisted affine transformations, similar to the SNES MODE7, allowing for effects like texture-mapped backgrounds and pseudo-3D perspective shifts.

### Summary

The **CGIA provides a powerful and flexible graphics pipeline**, combining display lists, multiple graphics modes, and an efficient **sprite multiplexing system**. By offloading rendering work to dedicated hardware, the X65 enables complex and visually rich applications while maintaining low CPU overhead. In the next chapter, we will explore **how to program the CGIA using assembly**, covering display list construction, sprite handling, and direct pixel manipulation.
