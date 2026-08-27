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

Four further targets drive the submodules' own builds. They are **opt-in** — none
of them is part of `all`, so plain `cmake --build build` still builds only the
book, which is all CI does. Ask for them by name:

    cmake --build build --target emu        # native emulator
    cmake --build build --target emu-wasm   # emulator for the web
    cmake --build build --target examples   # example .xex ROMs
    cmake --build build --target site       # refresh x65.zone (see below)

| Target | Produces | Needs |
| --- | --- | --- |
| `emu` | `build/emulator/emu` | the emulator's [dependencies](https://github.com/X65/emu#dependencies) |
| `emu-wasm` | `build/emulator-wasm/emu.{html,js,wasm}` | the [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html) on `PATH` |
| `examples` | `build/examples/src/*.xex` | the [cc65 toolchain](https://cc65.github.io/) on `PATH` |
| `site` | files copied into `x65.zone/emu/` | both of the above, plus `xex-filter.pl` |

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

The SID and POKEY players are a special case: they assemble to a bare player, and
the tune is a separate file that has to be loaded alongside it. `site` splices the
two together with [`xex-filter.pl`](https://www.vitoco.cl/atari/xex-filter/), which
is not vendored here — without it on `PATH` those ROMs are skipped, with a notice
at configure time.
