# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this repository is

`X65/X65` is the umbrella ("mono") repository for the **X65 microcomputer** — a modern 8-bit machine built around a WDC 65C816 with custom chips implemented in software on RP2350 microcontrollers.

It builds exactly one thing of its own: **the X65 Book** in `book/`, published to <https://docs.x65.zone>. Everything else is a git submodule pointing at an independent GitHub repo with its own build, CI and history.

The reason this checkout exists is that each child repo builds and passes CI on its own, so **contradictions *between* them are invisible from inside any one of them**. Duplicated register maps, vendored headers that drifted, docs describing behavior no implementation has. Treat cross-repo agreement as part of the deliverable, not just per-repo correctness.

| Path | Upstream | Contents |
| --- | --- | --- |
| `book/` | (this repo) | Sphinx/MyST book — hardware reference + 65816 programming guide |
| `firmware/` | `X65/firmware` | RP2350 firmware: three targets (`x65_north`, `x65_south`, `x65_audio`) — **the authoritative implementation of the machine** |
| `emulator/` | `X65/emu` | `emu`, the C/C++ emulator (chips-style, sokol + Dear ImGui + SDL3) |
| `examples/` | `X65/examples` | 65816 assembly / cc65 example programs producing `.xex` files |
| `schematic/` | `X65/schematic` | KiCad boards (`protoA`/`protoB`/`protoC`) + datasheets |
| `SGU-1/` | `X65/SGU-1` | Standalone SGU-1 sound core (`sgu.c`/`sgu.h`) + full register map in its README |
| `x65.zone/` | `X65/x65.github.io` | Jekyll marketing site (branch `master`), includes the WASM emulator under `emu/` |
| `_github/` | `X65/.github` | GitHub org profile |
| `reference/` | — | **Not a submodule.** Third-party datasheets (W65C816, RP2350, PCAL6416A, WS2812B, DVI…) for background reading only |

## Working across submodules

- `git submodule update --init` at the root populates the top level only. **Nested submodules are not pulled** — the emulator will not configure until `git -C emulator submodule update --init --recursive` (it needs `ext/SDL`, `ext/firmware`, `ext/sgu-1`, `ext/sokol`, `ext/imgui`, `ext/doctest`, `ext/cppdap`, `ext/speexdsp`), and the firmware needs its own init for `src/tinyusb`, `src/littlefs`, `src/pico_hdmi`.
- A change to firmware/emulator/examples/etc. is a commit **in that repo**. Push it there, then commit the moved gitlink here (the root history calls these "Update submodules"). The root repo never carries source changes for a child project.
- Submodules track branch `main`, except `x65.zone` which tracks `master`.

## Build and test

### The book (the only root-level build)

```sh
cmake -S . -B build
cmake --build build
```

Pipeline: Sphinx `-b dirhtml` over `book/` → `build/book/sphinx/`. Requires `sphinx-build` and `pip install -r book/requirements.txt`. (A Doxygen→Breathe stage used to feed firmware headers into the book; it was removed because no chapter ever used a `{doxygen*}` directive — the book is prose, and register tables are maintained by hand against the canonical sheet.)

Worth knowing:

- **Incremental rebuilds are dependency-tracked, so trust them.** `book/CMakeLists.txt` globs every `*.md` plus
  `_static/` and `_templates/` (with `CONFIGURE_DEPENDS`, so a newly added chapter is picked up without re-running
  `cmake`), and the Sphinx command touches `index.html` afterwards because Sphinx leaves it alone when its own
  doctree cache says nothing changed. Deleting a chapter is the one case the build system cannot notice — file
  removal never makes an output stale — so `rm -rf build/book/sphinx` after deleting a page.
- **Sphinx warnings are build errors.** The book builds warning-free and `book/CMakeLists.txt` passes `-W`, so any
  warning fails the build outright — there is no baseline to diff against. It also passes `-E`, which is what makes
  `-W` trustworthy: Sphinx only re-resolves documents it thinks are stale, so with a warm doctree cache a warning
  gets reported once and then silently vanishes on the next run, leaving a broken build that "passes" on retry.
  `-E` re-reads everything each time and costs nothing measurable (~3.5s for the whole book). If a Sphinx upgrade
  starts warning about something unrelated, fix it or drop the flag — do not reintroduce a tolerated baseline.
- The custom command's output is a `.sphinx-stamp` inside `build/book/sphinx/`, not `index.html`: under `-W` Sphinx
  writes its output *and then* exits non-zero, so keying off `index.html` would let a failed build look up to date.
  The stamp is only touched when `sphinx-build` returns 0, and lives in the output tree so `rm -rf build/book/sphinx`
  still forces a rebuild.
- **Heading anchors exist down to level 4** (`myst_heading_anchors = 4`), so `[text](../1/4_graphics.md#a-heading)`
  works. Em dashes are a trap: MyST strips the `—` but keeps the surrounding spaces, so a heading like
  `MODE0 and MODE1 — Paletted Modes` is `#mode0-and-mode1--paletted-modes` — **two** dashes, even though the id in
  the emitted HTML has one.
- **```asm fences are lexed as ca65**, not GNU as — `conf.py` rebinds the `asm` lexer, because the book's assembly
  is all cc65 syntax and Pygments' default `asm` lexer fails on `.struct`, `::` and `|`.

### Emulator

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release   # inside emulator/
cmake --build build --parallel
build/emu roms/SOTB.xex
```

Tests use doctest + CTest and build with the normal target list:

```sh
ctest --test-dir build --output-on-failure
ctest --test-dir build -R ArgsTest                     # one suite
build/src/tests/argstest --test-case="*crt*"           # one case
build/src/tests/argstest --list-test-cases
```

Suites: `CPUTest`, `ArgsTest`, `RingBufferTest`, `SGU1Test`, plus Linux-only ROM-driven CPU suites (`AllSuiteA`, `CPU816Suite`, `WaiInterrupt`) run through `src/tests/cpuemu.c`. The 3 GB SingleStepTests/65816 corpus is not vendored — `tools/fetch-sst65816.sh`, then configure with `-DSST65816_DIR=<path-to-v1>`.

Linux deps: `libX11-devel libXi-devel libXcursor-devel mesa-libEGL-devel alsa-lib-devel libunwind-devel` (Fedora names). WASM via `emcmake cmake`.

### Emulator headless scripting — use this to verify 65816 code

`emu --script FILE` drives the machine deterministically at 60 Hz with no keyboard: `run`, `until <addr>`, `joy`, `shot "f.png"`, `crc`/`expect-crc`, `dump`, `peek`/`poke`, `regs`, `cgia`, `trace`, `exit`. A failed check exits 1, so scripts double as CI smoke tests. Verbs are documented in `emulator/src/script.h`.

```sh
xvfb-run -a build/emu --disable-gui --script drive.scr roms/game.xex
build/emu --screenshot out.png --frames 120 roms/game.xex   # shorthand
```

Other useful flags: `--break EA|42|B8` (stop on opcode), `--dap` (Debug Adapter Protocol, Linux only). The WASM build takes the same options from the URL query string (`emu.html?file=roms/x.xex&crt=1,2,3&fullscreen`).

### Examples (65816 programs)

Needs the [cc65](https://cc65.github.io/) toolchain on `PATH`; `examples/cc65-toolchain.cmake` wires it up.

```sh
cmake -S . -B build && cmake --build build          # inside examples/
```

Each program is declared with the `add_xex(<name>)` helper in `examples/src/CMakeLists.txt` and links against `x65.cfg` (Atari-format output, `$0200` start, `INFO` at `$FC00`, vectors at `$FFE0`). Builds emit `.xex` plus `.lst`, `.lbl`, `.map` and `.dbg` next to it. Load the resulting `.xex` in the emulator to test.

### Firmware

Raspberry Pi Pico SDK 2.2.0 / arm toolchain (`gcc-arm-none-eabi`, `libnewlib-arm-none-eabi`); board `pimoroni_pga2350`, all three targets run at 336 MHz and are `copy_to_ram`. Standard `cmake -S . -B build && cmake --build build` with the SDK available. Flash/debug with a Picoprobe:

```sh
openocd -f interface/cmsis-dap.cfg -c "adapter speed 5000" -f target/rp2350.cfg \
        -c "program build/src/x65_north.elf verify reset exit"
picocom -b 115200 /dev/ttyACM0
```

### Website

`cd x65.zone && bundle install && bundle exec jekyll serve`. Deploys from `master` via GitHub Pages.

## Architecture

### The machine

W65C816 over a real CPU bus, 16 MB flat PSRAM (no banking), DVI-D output at 384×240, 256-colour palette. The custom chips are *software* running on RP2350s, which is why the firmware — not the book, not the emulator — is the specification.

`PHI2` has no crystal: NORTH synthesises it in PIO, so **the clock is a firmware setting, not a rating**, and throughput is bounded by PSRAM latency anyway. Don't quote a single headline MHz figure — the project has given three different ones in the past (`~7MHz`, `~4.2MHz`, `~3.14MHz`). The book now says `~3.1–4.2MHz` with a footnote; the emulator hardcodes `X65_FREQUENCY 3140000`.

### Three firmware personalities, one bus

- **NORTH** (`firmware/src/north/`) — the RIA (RP816 Interface Adapter, derived from RP6502). Drives the 65816 bus via PIO (`north/cpu.pio`), owns PSRAM access, USB host (HID + mass storage), FatFS/LittleFS, the monitor (`mon/`), the fastcall API surface (`api/`), Wi-Fi/BT via CYW43, and system services (`sys/`).
- **SOUTH** (`firmware/src/south/`) — the **CGIA** video chip (`south/cgia/`, with hand-written ARM assembly scanline encoders `cgia_encode.S` / `cgia_sprites.S`), HDMI/HSTX output, the VT terminal (`term/`), and audio PIO plumbing.
- **AUDIO** (`firmware/src/audio/`) — the SGU-1 sound generator, I²S out to the codec.

They talk over the **PIX bus** (Pico Information eXchange), defined once in `firmware/src/pix.h` + `pix.pio` and included by all three: message types (`PIX_MEM_WRITE`, `PIX_DMA_WRITE`, `PIX_DEV_*`), device ids (`PIX_DEV_RIA`/`VPU`/`SPU`/`MISC`) and per-device command enums. When adding cross-chip functionality, extend `pix.h` and both endpoints.

CGIA is display-list driven (ANTIC-style, decoded in `south/cgia/cgia.h`): instructions load memory/colour/charset pointers or emit a mode row; modes 0–3 are palette/attribute text and bitmap, 6 is Hold-and-Modify, 7 is affine chunky. Four independent planes, hardware scrolling, sprites, and three kinds of raster interrupt.

### Emulator mirrors the same chips

`emulator/src/chips/` implements `w65c816s`, `cgia`, `ria816`, `sgu1`, `tca6416a`, `m6526` as [floooh/chips](https://github.com/floooh/chips)-style pin-mask tick functions; `src/systems/x65.c` wires them into a machine and `src/ui/` provides ImGui debuggers per chip. The emulator build **includes firmware headers directly** (`ext/firmware/src/{south,north,audio}` are on the include path) and compiles the shared `ext/sgu-1/sgu.c`, so its register layouts cannot drift from the firmware's. Note this only holds for the emulator — see below. The 65816 core is generated — run `./w65c816s_gen.sh` from `emulator/src/codegen/` to regenerate `chips/w65c816s.h`; edit the generator, not the output.

### Memory map

Bank 0's top 1 KB is MMIO; everything else is PSRAM. `$FC00–$FDFF` expansion (4 cards × 128 bytes),
`$FEC0–$FEFF` SGU-1 (bank-switched 64-byte window), `$FF00–$FF7F` CGIA, `$FF80–$FF97` GPIO expander,
`$FF98` timers, `$FFA0` RGB LEDs, `$FFA8` buzzer, `$FFB0` USB HID, `$FFC0–$FFFF` RIA (math, TOD, DMA, file
descriptors, UART, IRQ controller, fastcall API at `$FFF0`, and both 65816 vector tables).

**The canonical source is the live spreadsheet**, not the book and not any header: <https://tinyurl.com/x65-memory-map>.
Fetch it as CSV rather than scraping the HTML — note it labels multi-byte registers on their **high** byte, the way
the CPU vectors are listed:

```sh
curl -sSL -o mm.csv "https://docs.google.com/spreadsheets/d/1mADeuKo_zZCQmT42eyEhKunW5CIMZ9LZTW8EDv7rU7w/export?format=csv&gid=0"
```

`book/A/A_memory_map.md` mirrors it and `book/A/E_cheat_sheet.md` summarises the hot registers; both have been wrong
before, so re-derive from the sheet or from `firmware/` when it matters.

## Shared contracts that drift

The same facts are written down in several places, and the copies are not generated:

- **`examples/src/cgia.h` and `examples/src/cgia.asm` are hand-vendored copies** of
  `firmware/src/south/cgia/cgia.h`. Nothing keeps them in step. After touching the firmware CGIA layout,
  `diff firmware/src/south/cgia/cgia.h examples/src/cgia.h` — the only expected differences are `#pragma once` and
  the `// ---- internals ----` block at the end.
- **The SGU-1 register map lives in four places**: `SGU-1/sgu.h` (the core), `SGU-1/README.md` (a full prose table),
  `emulator/src/chips/sgu1.c` (the service bank), and `book/A/A_memory_map.md`.
- **`emulator/ext/sgu-1` is pinned behind this checkout's `SGU-1`** — the emulator has `9d6bd93`, the umbrella has
  `eb71cbf`.
- **CGIA register offsets are `offsetof` into `struct cgia_t`**, so they are easy to get wrong by hand. Compute them
  rather than counting: compile a throwaway `printf("%zu", offsetof(struct cgia_t, order))` against the real header.

## Sources of truth

When a fact about the machine is in question, resolve in this order (from `book/AGENTS.md`):

1. `firmware/` — if the book and the firmware disagree, **the book is wrong**.
2. `schematic/` — board-level wiring, pinouts, connectors, but **only for released hardware**. The tree holds
   `protoA`/`protoB`/`protoC`; current development boards are unpublished work and will never appear here. Its
   silence about the Gen2 DEV-board is not a discrepancy to chase — ask, or take it from the firmware.
3. `emulator/` — a model, not the machine; generalize from it.
4. `examples/` — adapt snippets, don't link to them.
5. `reference/` — third-party datasheets; distill, never quote verbatim.

## Book conventions

`book/AGENTS.md` is the full guide; the essentials:

- MyST Markdown with `colon_fence`, `sphinx_design`, `sphinx_inline_tabs`, `sphinx_copybutton` and mathjax.
- Chapter files are globbed by `{toctree}`, so naming carries meaning: `book/1/N_topic.md` (single digit), `book/2/NN_topic.md` (zero-padded), `book/A/L_topic.md` (capital letter). Top heading is `# Chapter N: Title` / `# Appendix L: Title`.
- Cross-reference chapters with relative Markdown links (`[Chapter 4](../1/4_graphics.md)`). **Never hyperlink into a submodule path** — the published site does not expose those trees.
- New jargon goes into `book/A/B_glossary.md`, register/opcode summaries into `book/A/E_cheat_sheet.md`.
- Assembly blocks use ```` ```asm ````, C uses ```` ```c ````. Images live in `book/_static/`.

## Formatting

Each submodule carries its own `.clang-format` and they differ — firmware is WebKit-based with Allman braces and 4-space indent; the emulator is a 120-column custom style with attached braces. Format with the config of the tree you are editing.

## Non-obvious invariants

Things that look like bugs but are deliberate — confirm before "fixing" them:

- **SGU-1 is muted after reset.** `svc_master_vol` resets to `0` and gates the whole mix, so the chip is silent until
  software raises it. Audio hardware must not blast the user with whatever the register file powered up holding;
  unmuting is the OS's job. `emulator/doc/sgu-service-bank.md` used to claim the opposite.
- **`ria816_reset()` must not clear `reg[]`.** The 65816 vector tables live in that array (`$FFE4–$FFFF` is
  `reg[0x24..0x3F]`), so memset-ing it on reset wipes the reset vector. This is also why `EXTIOCTL` survives a warm reset.
- **In-tree work-order markdown is aspirational, not spec.** Files like `emulator/doc/sgu-service-bank.md` are drafts
  written *before* implementation. The source-of-truth list above names implementations; prose docs aren't on it.
  Never change working code to match one — ask first.

## Known rough edges

- `firmware/src/CMakeLists.txt` still lists `audio/snd/sgu.c` for the `x65_audio` target, but that file was removed from the firmware repo (HEAD: "Remove SGU cound core code") as the SGU core moves into the standalone `SGU-1` repo. The audio target will not link until that migration lands.
- `emulator/ext/*` submodules are uninitialized in a fresh checkout; the emulator's CMake failure modes look like missing SDL3/imgui rather than "run submodule update".
- **The firmware decodes nothing below `$FEC0`.** NORTH's `sys/ria.c` handles `$FEC0–$FFFF` and lets everything in
  `$FC00–$FEBF` fall through, so the expansion window and its `EXTIOCTL` (`$FFF6`) gating exist only in the book and
  the emulator. `$FFF6` is currently an inert byte in the RIA register file.
- The `$FF80–$FF97` GPIO expander window is documented but **not decoded by firmware** — reads return `$FF`.
- The emulator carries an `X65_IO_MIXER_BASE` (`$FEB0`) with a `FIXME: MIXER_CS`; no mixer appears on the canonical
  memory-map sheet.
