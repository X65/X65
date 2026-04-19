# Chapter 11: Graphics Programming

```{note}
This chapter is still under construction. The display-list, plane-register, and
sprite reference sections below are aligned with the current CGIA firmware; the
worked examples and tutorial sections are ongoing work.
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
- **3** – Load new memory pointers. **Bits 4-7** are flag bits selecting which scan pointers will be updated; the matching 16-bit values follow the instruction byte in the order listed.
  - Bit 4: Load Memory Scan (LMS) – Points to screen data.
  - Bit 5: Load Foreground Scan (LFS) – Points to foreground color data.
  - Bit 6: Load Background Scan (LBS) – Points to background color data.
  - Bit 7: Load Character Generator (LCG) – Points to character shape definitions.
- **4** – Load an 8-bit value into a plane register. Each plane has 16 multipurpose registers, indexed 0 - 15.
  - Bits 7-4: Register index.
- **5** – Load a 16-bit value into a plane register. Plane registers can also be addressed as eight 16-bit values, indexed 0 - 7.
  - Bits 6-4: Register index.
- **6**, **7** – Reserved (TBD).

#### Mode Row Instructions (8-F)

Mode row instructions define the type of graphics displayed on a scanline. The **lower three bits (0-2)** select the mode, while **bit 3** (always set in this group) differentiates them from control instructions.

- **8** – **Palette Text/Tile Mode** – Character-based graphics where each cell picks colors from the active palette.
- **9** – **Palette Bitmap Mode** – Direct pixel-based graphics drawn through the active palette.
- **A** – **Attribute Text/Tile Mode** – Character-based graphics with per-cell foreground and background color attributes (VIC-II style).
- **B** – **Attribute Bitmap Mode** – Direct pixel-based graphics with per-cell color attributes.
- **C**, **D** – Reserved (TBD).
- **E** – **HAM6 (Hold-and-Modify) Mode** – Similar to **Amiga HAM**, where pixel colors can be modified based on previous pixels, enabling a larger color range. See [HAM Encoding Format](#ham-encoding-format) below.
- **F** – **Affine Transform Chunky Pixel Mode (MODE7)** – Inspired by **SNES MODE7**, allowing **rotation and scaling of graphics** for pseudo-3D effects.

Mode row instructions also carry two flag bits that modify how the mode is rendered:

- **Bit 4** (`DOUBLE_WIDTH`) – Each logical pixel covers two physical screen pixels horizontally.
- **Bit 5** (`MULTICOLOR`) – Switches text and bitmap modes to a **4-color-per-cell** representation, where each byte encodes four pixels (similar to C64 multicolor mode).

Additionally, **bit 7** in any mode row instruction triggers a **Display List Interrupt (DLI)**, allowing for **scanline-specific effects, color changes, or sprite updates**.

### Using the Display List

A typical display list sequence begins with a **Load Memory Scan (LMS) instruction**, setting the screen’s base address. Subsequent instructions configure **row modes, colors, and additional graphics settings**. The final instruction often loops back to the beginning, ensuring continuous display refresh.

By leveraging the **CGIA’s display list system**, developers can efficiently mix **text, tiles, bitmaps, and advanced graphical effects** in a single frame without CPU overhead. This design enables **smooth scrolling, parallax effects, sprite multiplexing, and dynamic screen updates**, making the X65’s graphics system a powerful tool for retro-style computing and game development.

## Plane Registers Interpretation

Each **CGIA plane** has **16 associated registers**, but their interpretation depends on the selected **plane type** and **graphics mode**. The last **display list mode instruction** determines how the plane registers are utilized. The register structure varies based on whether the plane is handling **background graphics, HAM mode, affine transformations, or sprites**.

### Background Plane Registers

- **flags** _(8-bit)_ – Configuration flags (see below).
- **border_columns** _(8-bit)_ – Number of border columns on each side.
- **row_height** _(8-bit, unsigned)_ – Height of each row in raster lines.
- **stride** _(8-bit, unsigned)_ – Memory stride for addressing screen data.
- **scroll_x, scroll_y** _(8-bit, signed)_ – Fine scrolling values.
- **offset_x, offset_y** _(8-bit, signed)_ – Base offsets for plane positioning.
- **color[8]** _(8-bit each)_ – Eight plane-wide colors, used for borders, multicolor cells, and shared palette entries depending on the active mode and flags.

#### Background Plane Flags

- **Bit 0** – Color 0 is transparent.
- **Bits 1-2** – Reserved.
- **Bit 3** – Border is transparent.
- **Bit 4** – Double-width pixel mode.
- **Bit 5** – Multicolor pixel mode.
- **Bits 6-7** – Pixel bit-depth: `00` = 1 bpp / 2 colors, `01` = 2 bpp / 4 colors, `10` = 3 bpp / 8 colors, `11` = 4 bpp / 8 colors + half-bright.

```text
PLANE_MASK_TRANSPARENT        %00000001
PLANE_MASK_BORDER_TRANSPARENT %00001000
PLANE_MASK_DOUBLE_WIDTH       %00010000
PLANE_MASK_MULTICOLOR         %00100000
PLANE_MASK_PIXEL_BITS         %11000000
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

When a plane is configured as a **sprite plane**, its 16 plane registers carry these fields (the remaining bytes are reserved):

- **active** _(8-bit)_ – Bitmask indicating which of the eight sprites are active.
- **border_columns** _(8-bit, unsigned)_ – Number of border columns.
- **start_y, stop_y** _(8-bit, unsigned)_ – Vertical range where sprites are visible (clipping window).

The sprite plane does **not** consume a display list. Instead, the plane's `offsetN` register points at a **sprite descriptor table** in memory.

#### Sprite Descriptor (16 bytes)

Each entry in the sprite descriptor table is exactly 16 bytes, laid out as follows:

| Offset | Field             | Size | Notes                                                                                       |
| ------ | ----------------- | ---- | ------------------------------------------------------------------------------------------- |
| 0      | `pos_x`           | i16  | Signed X position (can be negative for off-screen entry).                                   |
| 2      | `pos_y`           | i16  | Signed Y position.                                                                          |
| 4      | `lines_y`         | u16  | Sprite height in raster lines.                                                              |
| 6      | `flags`           | u8   | Width / mode bits (see below).                                                              |
| 7      | _reserved_        | u8   | —                                                                                           |
| 8      | `color[3]`        | 3×u8 | Colors for indices 1, 2, 3 (index 0 is always transparent).                                 |
| 11     | _reserved_        | u8   | —                                                                                           |
| 12     | `data_offset`     | u16  | 16-bit pointer (within the sprite bank) to the sprite pixels.                               |
| 14     | `next_dsc_offset` | u16  | Pointer to the next descriptor after `lines_y` rows; built-in **sprite multiplexer** entry. |

The **flags** byte:

- **Bits 0-2** – Width minus one, in bytes (1–8 bytes = 8–64 pixels).
- **Bit 3** – Reserved.
- **Bit 4** – Double-width.
- **Bit 5** – Multicolor.
- **Bit 6** – Mirror X.
- **Bit 7** – Mirror Y.

```text
SPRITE_MASK_WIDTH        %00000111
SPRITE_MASK_DOUBLE_WIDTH %00010000
SPRITE_MASK_MULTICOLOR   %00100000
SPRITE_MASK_MIRROR_X     %01000000
SPRITE_MASK_MIRROR_Y     %10000000
```

By utilizing **dynamic plane register interpretation**, the CGIA enables **highly versatile graphics rendering**, allowing the X65 to seamlessly mix **background layers, sprites, advanced color manipulation, and affine transformations** in real time.

### Worked Example: Eight Multicolor Sprites

The following adapted snippet (from the `sprites` example program) sets up plane 0 as a sprite plane with all eight sprites enabled, then fills a descriptor table by stepping through it 16 bytes at a time. Each sprite is `SPRITE_WIDTH` bytes wide (so `SPRITE_WIDTH * 8` pixels), `SPRITE_HEIGHT` lines tall, and uses three shared colors plus transparency on color index 0:

```asm
.define SPRITE_WIDTH   4
.define SPRITE_HEIGHT  26

reset:
    sei                     ; disable IRQs while we reconfigure CGIA

    lda #0                  ; disable all planes during setup
    sta CGIA::planes

    lda #145                ; pick a border/background color
    sta CGIA::back_color

    ; point plane 0 at the sprite descriptor table
    lda #<sprite_descriptors
    sta CGIA::offset0
    lda #>sprite_descriptors
    sta CGIA::offset0 + 1

    ; sprite-plane-specific registers
    lda #%11111111          ; activate all 8 sprites
    sta CGIA::plane0 + CGIA_SPRITE_REGS::active
    lda #0                  ; no border, sprites visible across the whole screen
    sta CGIA::plane0 + CGIA_SPRITE_REGS::border_columns
    sta CGIA::plane0 + CGIA_SPRITE_REGS::start_y
    sta CGIA::plane0 + CGIA_SPRITE_REGS::stop_y

    ; ... fill 8 × 16-byte descriptors at sprite_descriptors ...

    lda #%10000000          ; enable Vertical Blank NMI
    sta CGIA::int_enable

    lda #%00010001          ; plane0 = enabled, type = sprite
    sta CGIA::planes
```

Per-descriptor flags can mix the modifier bits freely. For example:

```asm
    ; multicolor + mirror X, sprite is SPRITE_WIDTH bytes wide
    lda #(SPRITE_MASK_MULTICOLOR | SPRITE_MASK_MIRROR_X | (SPRITE_WIDTH-1))
    sta sprite_descriptors + 4*CGIA_SPRITE_DESC_LEN + CGIA_SPRITE::flags

    ; multicolor + mirror Y
    lda #(SPRITE_MASK_MULTICOLOR | SPRITE_MASK_MIRROR_Y | (SPRITE_WIDTH-1))
    sta sprite_descriptors + 5*CGIA_SPRITE_DESC_LEN + CGIA_SPRITE::flags

    ; multicolor + double-width pixels
    lda #(SPRITE_MASK_MULTICOLOR | SPRITE_MASK_DOUBLE_WIDTH | (SPRITE_WIDTH-1))
    sta sprite_descriptors + 3*CGIA_SPRITE_DESC_LEN + CGIA_SPRITE::flags
```

A VBL NMI handler can then animate sprites by writing fresh `pos_x` / `pos_y` values into individual descriptors — no per-frame display-list rebuild required.

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

## Implementing Paletted Modes (MODE0 / MODE1)

MODE0 and MODE1 are the CGIA's **paletted** modes: the colour palette for the plane lives in the upper half of the plane's 16 registers (`color[0..7]`), giving eight palette entries per plane. No separate foreground/background scan pointers are needed — the savings in both memory traffic and plane-register programming are the whole point of these modes.

Both modes pick pixel bit depth through the two high bits of the plane `flags` register (`PLANE_MASK_PIXEL_BITS`):

| `PIXEL_BITS` | bpp | Colours per pixel | MODE0 glyph budget | MODE1 bitmap layout      |
| ------------ | --- | ----------------- | ------------------ | ------------------------ |
| `%00`        | 1   | 2                 | Full 256 glyphs    | 1 byte = 8 pixels        |
| `%01`        | 2   | 4                 | 128 glyphs         | 1 byte = 4 pixels        |
| `%10`        | 3   | 8                 | 64 glyphs          | 4 pixels packed in 3 B   |
| `%11`        | 4   | 8 + half-bright   | 32 glyphs          | 2 pixels per nibble      |

### MODE1 — Paletted Bitmap

In MODE1, the bitmap bytes **index the palette directly**:

- **1 bpp**: each bit picks palette entry 0 or 1. Eight pixels per byte.
- **2 bpp**: each pair of bits picks one of palette entries 0–3. Four pixels per byte.
- **3 bpp**: pixels are bitpacked HAM-style — four pixels in three bytes. CGIA decodes 24 bits as `4 × 6 bits` and each 6-bit group's low three bits index the palette (the high three are reserved for mode-specific work; for pure bitmap use they default to zero).
- **4 bpp**: high bit of each nibble is the **half-bright flag**, bits 2:0 select palette entry 0–7.

Typical setup: pick the palette, set bit depth, point the LMS scan pointer at a packed bitmap, pick a MODE1 mode-row instruction, and let the beam do the rest.

```asm
    ; Palette: eight entries in plane 0's color[0..7]
    lda #$10            ; dark blue
    sta CGIA::plane0 + CGIA_BG_REGS::color + 0
    lda #$15            ; bright blue
    sta CGIA::plane0 + CGIA_BG_REGS::color + 1
    lda #$22
    sta CGIA::plane0 + CGIA_BG_REGS::color + 2
    ; ... populate color[3..7] ...

    ; 2 bpp paletted bitmap, solid cells
    lda #PLANE_BITS_2BPP
    sta CGIA::plane0 + CGIA_BG_REGS::flags

    ; LMS at the bitmap
    lda #<pic_pixels
    sta dl_lms_lo
    lda #>pic_pixels
    sta dl_lms_hi
```

The mode-row instruction itself uses nibble `9` (MODE1 = slot 9 in the display list mode-row group).

### MODE0 — Paletted Text/Tile

MODE0 borrows MODE1's palette-in-registers mechanism but keeps the character-generator indirection from the attribute text modes. Every screen cell is still one byte of character code plus a row-by-row fetch from the character generator, exactly as in MODE2. What is different is how colour is assigned per pixel:

- At **1 bpp**, the character-generator bit is the palette index. Palette entry 0 is "off", entry 1 is "on". Full 256-glyph fonts fit.
- At **higher bpp**, the character-generator bit is only the low bit of the palette index. The additional bits come from the **high bits of the character code** — the font "pays" for colour by giving up glyph-code space.

For **2 bpp** the top bit of the character code becomes the second colour-index bit; the font is limited to codes `$00..$7F` (128 glyphs). For **3 bpp** the top two bits become colour bits and the font gets `$00..$3F` (64 glyphs). At **4 bpp** the top three bits participate: the very top bit is the half-bright flag, the next two bits join the colour index alongside the char-gen bit, and the remaining five bits (`$00..$1F`) identify the glyph — 32 glyphs total.

In practice MODE0 is almost always used at 1 bpp (a crisp two-colour font over a fixed palette entry) or 2 bpp (four-colour decorative text), with higher bpps reserved for tilemap/backdrop work where glyph count is not the limiting factor.

### The half-bright transform (4 bpp)

At 4 bpp, the high bit of the colour index does _not_ pick between eight palette entries (there are only eight). It XORs bit 2 of the palette index, which swaps the "dark" and "bright" halves of the palette row in the 256-colour CGIA palette. Visually this produces brighter/darker twins without needing 16 explicit palette slots:

```text
base palette bits 2:0 → 0 1 2 3 4 5 6 7
half-bright flag OFF → pick color[idx]
half-bright flag ON  → pick color[idx XOR 4]   ; flips to the other half
```

If the programmer has loaded the palette so that entries 0–3 are the dark variants of entries 4–7 (a common convention), the result is 16 visible shades with a free up/down "brighter / darker" control per pixel. If the palette is loaded arbitrarily, the high bit just becomes a second palette selector — still useful, but less convenient.

### MODE0 multi-color text

MODE0 supports the same `MULTICOLOR` flag as the attribute text modes, and with the same effect: two character-generator bits feed the colour decision per screen pixel, so cells are **4 pixels wide** instead of 8. Multi-color MODE0 is restricted to 2 bpp and 3 bpp:

- 1 bpp multi-color is impossible (there is already only one bit to work with).
- 4 bpp multi-color would conflict with the half-bright transform (two bits of char-gen plus two bits of stolen code leaves no room for the half-bright high bit), so firmware does not support it.

### 80-column mode

A particularly useful side effect of 4-pixel-wide multi-color cells is the 80-column mode. On the standard 320-pixel logical screen, 320 ÷ 4 = 80 cells across. Any multi-color text mode without the double-width flag therefore yields 80 columns out of the box:

```asm
    ; MODE0 in multi-color, 2 bpp, no double-width
    lda #(PLANE_BITS_2BPP | PLANE_MASK_MULTICOLOR)
    sta CGIA::plane0 + CGIA_BG_REGS::flags

    ; Pick palette entries so that "ink" is a clear colour pair
    lda #$00                ; background (color[0])
    sta CGIA::plane0 + CGIA_BG_REGS::color + 0
    lda #$00                ; unused in a two-tone 80-col font
    sta CGIA::plane0 + CGIA_BG_REGS::color + 1
    lda #$00                ; unused
    sta CGIA::plane0 + CGIA_BG_REGS::color + 2
    lda #$FE                ; ink  (color[3])
    sta CGIA::plane0 + CGIA_BG_REGS::color + 3

    ; Use a 4×8 font whose glyphs only fire pixels with multi-color
    ; patterns `00` (background) and `11` (ink)
    lda #<font_4x8
    sta dl_lcg_lo
    lda #>font_4x8
    sta dl_lcg_hi
```

A 4×8 font designed for this purpose uses only the `00` and `11` multi-color codes, producing a clean, readable two-tone 80-column display — suitable for code listings, terminal UI, and long-text screens — while keeping the full one-byte-per-character storage of normal text modes. All the usual display-list mechanics (scrolling, DLI, mid-frame mode switches) still apply.

### Double-width text across all modes

The `PLANE_MASK_DOUBLE_WIDTH` flag (bit 4 of the plane flag byte, also encoded as `CGIA_DL_DOUBLE_WIDTH_BIT` in the mode-row instruction) doubles the horizontal pixel size of any text mode. It pairs naturally with multi-color for the chunky "rectangle-pixel" look (C64 multi-color text style), and stands on its own with non-multi-color text for the Atari "wide character" modes. Because the flag lives in both the plane register _and_ the display-list mode-row instruction, switching double-width on or off mid-screen is as easy as writing the right opcode on the target row.

## Creating Mixed-Mode Display Lists

## Plane ordering

The CGIA supports up to **four graphics planes**, which can be layered to create complex visual effects. The order in which these planes are rendered is crucial for achieving the desired appearance, especially when combining different graphics modes.

It is beneficial to be able to change the order of planes without reconfiguring the entire plane settings. The CGIA allows for dynamic plane ordering through the use of a **Plane Order Register**.

### Plane Order Register

With four planes, there are 24 possible permutations for their rendering order. The Plane Order Register is a register that picks the order in which the planes are drawn.

The order uses the Steinhaus–Johnson–Trotter (adjacent-swap “revolving door”) order. Each step swaps one adjacent pair, so “next” and “previous” are obvious.

Starting at 1234, the 24 permutations:

```
1234 1243 1423 4123 4132 1432
1342 1324 3124 3142 3412 4312
4321 3421 3241 3214 2314 2341
2431 4231 4213 2413 2143 2134
```

Rule to get the next permutation from the current one:

1. Give every element a direction (initially all point left).
2. Find the largest “mobile” element: one whose arrow points to a smaller neighbor.
3. Swap it with that neighbor in its arrow direction.
4. Reverse the arrows of all elements larger than the moved one.
5. Repeat until no mobile element exists.

To go backward, reverse the rule. This is a Gray-code analogue for permutations on the permutohedron using adjacent transpositions.
