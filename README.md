# X65

Modern 8-bit microcomputer

X65 is an 8-bit microcomputer for the modern era.
It uses best of breed components and strives to keep 8-bit feeling,
while being usable for daily basis computing activities.

This is the umbrella repository. Every part of the machine — firmware, emulator,
examples, schematics, website — lives in its own repository and is pulled in here
as a submodule. What this repository builds itself is the **X65 Book**, published
at <https://docs.x65.zone>.

## Building

> [!TIP]
> This repository uses submodules.
> You need to do `git submodule update --init` after cloning, or clone recursively.
> That populates the top level only — the emulator has submodules of its own:
>
>     git -C emulator submodule update --init --recursive

### The book

The default build makes the book and nothing else:

    cmake -S . -B build
    cmake --build build

Needs `sphinx-build` and `pip install -r book/requirements.txt`.
The rendered site lands in `build/book/sphinx/`.

Sphinx warnings are errors here, so a broken cross-reference or a chapter missing
from a `{toctree}` fails the build rather than shipping.

### The submodules

Five further targets drive the submodules' own builds. They are **opt-in** — none
of them is part of `all`, so plain `cmake --build build` still builds only the
book, which is all CI does. Ask for them by name:

    cmake --build build --target emu           # native emulator
    cmake --build build --target emu-wasm      # emulator for the web
    cmake --build build --target examples      # example .xex ROMs
    cmake --build build --target example-data  # picture data for the ROMs that need it
    cmake --build build --target site          # refresh x65.zone (see below)

| Target | Produces | Needs |
| --- | --- | --- |
| `emu` | `build/emulator/emu` | the emulator's [dependencies](https://github.com/X65/emu#dependencies) |
| `emu-wasm` | `build/emulator-wasm/emu.{html,js,wasm}` | the [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html) on `PATH` |
| `examples` | `build/examples/src/*.xex` | the [cc65 toolchain](https://cc65.github.io/) on `PATH` |
| `example-data` | `build/example-data/*.xex` | a host C compiler |
| `site` | files copied into `x65.zone/emu/` | all of the above, plus `xex-filter.pl` |

Each one shells out to that project's own CMake rather than being absorbed into
this build — they use different toolchains, and their own dependency tracking is
the one that knows what needs rebuilding. Everything is built **into this build
tree**, so the submodule working directories stay clean and `git status` inside
them keeps meaning what it should.

A submodule that was never checked out fails with a message telling you what to
run, rather than with a confusing error from inside its own CMake.

### Publishing to x65.zone

The website serves a WebAssembly build of the emulator from `emu/`, and the ROMs
it offers from `emu/roms/`. Both are committed artifacts in that repository, not
build outputs of it, so they go stale unless something rebuilds and copies them.
The `site` target is that something: it builds `emu-wasm` and `examples`, then
copies the web emulator and the ROMs into the `x65.zone` checkout.

    cmake --build build --target site
    git -C x65.zone status

> [!IMPORTANT]
> `site` copies and stops. It stages nothing and commits nothing — x65.zone
> deploys from `master` through GitHub Pages, so review the diff and commit it
> yourself.

Unchanged files are left alone, so the diff only shows ROMs that actually moved.

Which ROMs get published, and under what names, is the table in
[`cmake/SiteRoms.cmake`](cmake/SiteRoms.cmake) — edit it to add one. It mirrors
the entries in the site's own catalogue (`x65.zone/_data/emu.yml`) that come from
the examples repository; every other ROM under `emu/roms/` is hand-made or from
elsewhere, and `site` never touches those.

Some examples do not assemble to a whole ROM and have to be merged with something
before they are worth publishing, which `site` does with
[`xex-filter.pl`](https://www.vitoco.cl/atari/xex-filter/). It is not vendored
here — without it on `PATH` those ROMs are skipped, with a notice at configure
time. There are two kinds:

- **The SID and POKEY players** assemble to a bare player; the tune is a separate
  file that has to be loaded alongside it, at the address the player was built
  against. `SITE_TUNE_ROMS` in `cmake/SiteRoms.cmake` is that table.
- **`sotb` and `mixed_modes`** assemble to code only. Their pictures live as C
  arrays in the examples repository and are written out as loadable blocks by the
  `example-data` target; `SITE_DATA_ROMS` says which blocks go with which demo,
  and in what order. Each demo's `.asm` header comment carries the same recipe by
  hand — keep the two in step, because nothing checks.

`4BB` is the one ROM on the site that cannot be rebuilt here at all: its picture
comes from a converter and a source image that are not in this tree, so `site`
leaves the committed file alone.
