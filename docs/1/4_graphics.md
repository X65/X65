# Chapter 4: Graphics and Display

## The Color Graphics Interface Adaptor (CGIA)

The **CGIA (Color Graphics Interface Adaptor)** is the graphics subsystem of the X65 microcomputer, merging design elements from both the **Atari ANTIC** and **Commodore VIC-II/TED** architectures. It provides a **display list system** similar to ANTIC while incorporating a **separate character color mapping system** akin to the VIC-II and TED chips. This combination allows for a flexible yet efficient graphics pipeline, offloading much of the rendering workload from the CPU.

### Principles of Operation

The CGIA is an **on-bus device**, with **memory-mapped registers**. While the current implementation is done using a microcontroller firmware, that's only one approach - it could also be realized in an **FPGA** or **custom ASIC design**.

CGIA has **128 registers**, mapped at `$FF00..$FF7F`, used to configure its operation. It is a **fetch master** on the south-side bus — once configured, it pulls display-list instructions, character data, color maps, and sprite data straight out of system memory each frame, without intervention from the 65C816.

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

At the beginning of each display frame, CGIA loads the first display list instruction and begins rendering. The instruction selects one of **8 display-mode slots**, currently assigned as follows:

| Slot | Name   | Family              | Description                                       |
| ---- | ------ | ------------------- | ------------------------------------------------- |
| 0    | MODE0  | Paletted text/tile  | Per-plane palette; no attribute memory needed     |
| 1    | MODE1  | Paletted bitmap     | Per-plane palette; direct pixel indexing          |
| 2    | MODE2  | Attribute text/tile | Per-cell foreground/background from scan pointers |
| 3    | MODE3  | Attribute bitmap    | Per-cell foreground/background from scan pointers |
| 4    | —      | Reserved            | —                                                 |
| 5    | —      | Reserved            | —                                                 |
| 6    | MODE6  | HAM6                | Hold-and-Modify, 4 pixels per 3 bytes             |
| 7    | MODE7  | Affine              | Chunky pixels with hardware affine transforms     |

CGIA then draws **8 pixels at a time** using fetched character, bitmap, or colour data, and repeats this process to fill the raster line.

This is done **separately for each of the 4 planes**, which are **composited** in one line color buffer.

Once a raster line is fully constructed, it is pushed to a **hardware rasterizer** that converts the data into **TMDS symbols** for DVI/HDMI output. The output signal is **768×480 @ 60 Hz**. By default each logical pixel is doubled in both axes — symbol sent twice, raster line repeated once — producing an effective **384×240** logical resolution. Two bits in the CGIA mode register opt in to higher fidelity: `HIRES` switches to 96-column (768 px) horizontal mode, and `INTERLACE` enables a 480-line vertical mode.

#### Sprite Plane Operation

If a plane is configured as a **sprite plane**, it doesn't use a display list. Instead, it uses a **sprite descriptor table**, starting from a configurable pointer. At the beginning of the frame, **eight sprite descriptors** are loaded. Each sprite descriptor sets the sprite screen position, width, height and modes (multicolor, pixel doubling, etc.).

On each raster line, CGIA checks which sprites overlap that line (from `y_pos` to `y_pos + height`). It then fetches and draws the appropriate pixels into the raster line, based on the current `x_pos`. Sprites are drawn in **descending order**, so lower-numbered sprites overwrite higher-numbered ones (i.e., sprite 1 has higher priority than sprite 2).

Once the sprite finishes drawing (the raster line reaches its bottom), the sprite descriptor is reloaded with new contents. This allows the sprite to be reused in a lower part of the screen with a different X position and appearance, creating a built-in sprite multiplexer.

### Real-Time, Non-Persistent Raster Line Rendering

This rendering process occurs **in real time**, at a **perfect 60 Hz**, for every line of the active picture. The DVI-D output is driven by the RP2350's HSTX block, clocked from the chip's second PLL rather than divided off the system clock — so the framerate stays at exactly 60 Hz regardless of CPU-side tuning, and VBI-synchronous software can rely on it. Importantly, there is **no persistent framebuffer**; every line is generated and transmitted immediately and is not stored in memory. A full framebuffer at the output resolution would consume hundreds of kilobytes, which is impractical for the X65's architecture.

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

* **Paletted text/tile (MODE0) and bitmap (MODE1):** No attribute memory; up to eight colours sit in the plane's registers and each pixel indexes that palette.
* **Attribute text/tile (MODE2) and bitmap (MODE3):** Per-cell foreground/background colours fetched via separate scan pointers — the classic VIC-II layout.
* **Multicolor mode flag:** A per-instruction bit on text and bitmap modes that switches to a **4-color-per-cell representation**, where each byte encodes four pixels. Because the cell narrows to 4 pixels wide, multi-color text on a 320-pixel screen yields **80 columns** out of the box.
* **Double-width flag:** A separate per-plane and per-instruction bit (`PLANE_MASK_DOUBLE_WIDTH` / `CGIA_DL_DOUBLE_WIDTH_BIT`) available on every text mode. It doubles the horizontal pixel size of each cell, producing the chunky "wide character" look from Atari 8-bit text modes; combined with multi-color, it gives the C64-style 8-pixel-per-byte rectangle pixels.

The X65 features a **256-color palette**, composed of 32 distinct hues, each available in 8 different brightness levels. Each mode can define **foreground and background colors per character/tile**, much like the VIC-II, but with additional flexibility due to CGIA's three separate scan pointers.

#### MODE0 and MODE1 — Paletted Modes

MODE0 (paletted text/tile) and MODE1 (paletted bitmap) share a single mechanism: instead of fetching per-cell colours through the scan pointers, they take their colours from an **8-entry palette** stored directly in the plane's own registers — specifically, the upper half of the sixteen plane registers (`shared_colors[0..7]`), where each byte is an index into the global 256-colour CGIA palette.

Both modes let software choose one of four **colour depths** via the plane's `PIXEL_BITS` field:

| bpp | Colours per pixel | MODE1 bitmap layout                         | MODE0 glyph budget |
| --- | ----------------- | ------------------------------------------- | ------------------ |
| 1   | 2                 | 1 pixel per bit (8 pixels per byte)         | 256 glyphs         |
| 2   | 4                 | 2 pixels per bit-pair (4 pixels per byte)   | 128 glyphs         |
| 3   | 8                 | 4 pixels packed into 3 bytes (HAM-style)    | 64 glyphs          |
| 4   | 8 + half-bright   | 2 pixels per nibble, high bit = half-bright | 32 glyphs          |

In MODE1 (bitmap) the raw bits index the palette directly. In MODE0 (text/tile) the character-generator bit for the current pixel becomes the **low** bit of the palette index — and the **high** bits of the palette index are "stolen" from the **high** bits of the character code byte. The font ROM is then addressed only by the bits that remain. So the same byte that picks a glyph also picks which slice of the palette that glyph paints with: consecutive ranges of character codes paint with consecutive palette pairs, while the char-gen bit picks the off/on colour within each pair.

Concretely:

* **1 bpp** — no bit is stolen; all 256 character codes are usable, every cell paints from `palette[0..1]`.
* **2 bpp** — bit 7 is stolen as palette bit 1. 128 glyphs (`$00..$7F`). Codes `$00..$7F` paint with `palette[0..1]`, codes `$80..$FF` paint with `palette[2..3]`.
* **3 bpp** — bits 7..6 are stolen as palette bits 2..1. 64 glyphs (`$00..$3F`). Four palette pairs across the four char-code quarters: `$00..$3F` → `palette[0..1]`, `$40..$7F` → `[2..3]`, `$80..$BF` → `[4..5]`, `$C0..$FF` → `[6..7]`.
* **4 bpp** — bits 7..5 participate (top bit is the half-bright flag, see below); 32 glyphs (`$00..$1F`).

The trade is per-mode and per-scene: text-heavy screens stay at 1 bpp, decorated tilemaps move up to 2 or 3 bpp, splash screens with rich colour can afford 4 bpp at the cost of a small glyph set.

**Half-bright (4 bpp).** At 4 bpp the character-code byte splits as `[HB | P2 P1 | g4 g3 g2 g1 g0]`: the very top bit is the **half-bright flag**, bits 6..5 are stolen as palette bits 2..1, and only the low five bits identify a glyph. The half-bright flag is *not* part of palette selection — it acts **after** the per-cell `shared_colors[]` lookup. The flag is one bit per cell; once the per-pixel palette index has chosen one of the eight `shared_colors[]` entries and that lookup has produced an 8-bit CGIA palette index, the half-bright flag XORs bit 2 of **that** CGIA index. In the CGIA's 256-colour palette (32 hue rows × 8 brightness levels) bit 2 of the index is a brightness bit, so flipping it lands on the same hue's brightness twin — turning the cell's colours into their brighter (or darker) counterparts. The flag applies to every pixel of the cell, so both colours used by a 4 bpp non-multi cell jump to their twins together. The net effect is 16 visible colours from 8 palette entries, with a built-in dark/bright pairing that works regardless of how `shared_colors[]` was loaded — much more useful than arbitrary "just dim it" schemes.

**MODE0 multi-color.** The multi-color flag is available on MODE0 at 2 bpp and 3 bpp. It takes **two** bits of the character-generator output per screen pixel rather than one, so cells are 4 pixels wide instead of 8 and the character set encodes two-bit patterns instead of on/off. The two char-gen bits become the **low two** bits of the palette index, and any remaining high bits of the palette index are stolen from the character code as before:

* **2 bpp multi** — no bit is stolen; the char-gen bits *are* the full 2-bit palette index. All 256 character codes are usable, every cell paints from `palette[0..3]`. (This is the configuration that yields the 80-column text mode — see below.)
* **3 bpp multi** — bit 7 of the character code is stolen as palette bit 2 (a "palette half" select). 128 glyphs (`$00..$7F`). Codes `$00..$7F` paint from `palette[0..3]`, codes `$80..$FF` paint from `palette[4..7]`.

1 bpp multi-color is impossible (we already take two bits by default) and 4 bpp multi-color makes no sense, as there is no way of telling which of four multi-colors needs to be shifted.

#### Double-width text and 80-column mode

Every text mode supports a per-plane **double-width** flag (`PLANE_MASK_DOUBLE_WIDTH`, bit 4 of the plane's flag register). Setting it doubles the horizontal pixel size of every cell, producing the chunky "wide character" look familiar from Atari and C64 text modes. In the display list this is commonly set and cleared by the `CGIA_DL_DOUBLE_WIDTH_BIT` in the mode-row instruction, so software can switch mid-screen.

The multi-color flag interacts with it cleanly: without double-width, multi-color text narrows the cell to 4×8 pixels, giving **80 columns** on the 320-pixel-wide screen. With a font that uses only the 00 and 11 multi-color codes (background and background-2), the result reads as a crisp monochrome 80-column mode — perfect for terminal-style UI, code listings, and long-text rendering — while staying fully byte-per-character compatible with the rest of the text-mode machinery.

#### Plane Order Register

A dedicated **plane order register** holds a permutation of the four plane indices, encoded compactly so any of the 24 possible Z-orderings of the four planes fits in a single byte. Writing to it on the fly — typically from a raster interrupt — lets a program shuffle which plane is on top without rebuilding any of the planes themselves, which is convenient for things like menu overlays, mid-frame UI, and parallax tricks where the foreground plane changes between scanlines.

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
* **Mirror X / Mirror Y:** Per-sprite flags that flip the rendered sprite horizontally or vertically without re-encoding the data.
* **Color index 0 transparency:** The first color is always transparent.

### Memory Banking for Graphics Rendering

The CGIA includes two configuration registers: **background\_bank** and **sprite\_bank**. These registers determine the high byte (bits 16–23) of the memory addresses used for rendering background and sprite graphics, respectively. This enables efficient graphics memory management, allowing both background and sprite data to reside in separate or shared memory banks.

### Advanced Graphics Modes

CGIA includes two special graphics modes that extend its capabilities beyond traditional tile and bitmap rendering:

* **HAM6 (Hold-And-Modify, mode 6):** Inspired by the Amiga HAM mode. HAM commands are 6 bits each, packing four screen pixels into three bytes; commands either select a base color or modify a single channel (R, G, or B) of the previous pixel by a signed delta, enabling more colors per screen at the cost of per-pixel precision.
* **MODE7 (affine, mode 7):** A chunky-pixel graphics mode with hardware-assisted affine transformations, similar to the SNES MODE7, allowing effects like texture-mapped backgrounds and pseudo-3D perspective shifts.

### Summary

The **CGIA provides a powerful and flexible graphics pipeline**, combining display lists, multiple graphics modes, and an efficient **sprite multiplexing system**. By offloading rendering work to dedicated hardware, the X65 enables complex and visually rich applications while maintaining low CPU overhead. In the next chapter, we will explore **how to program the CGIA using assembly**, covering display list construction, sprite handling, and direct pixel manipulation.
