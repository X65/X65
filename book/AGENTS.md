# AGENTS.md

## Purpose

This file guides AI coding agents working on **the X65 Book** — a Sphinx/MyST documentation project in [`book/`](./) that teaches the hardware and 65816-assembly programming of the X65 microcomputer. The book is published at <https://docs.x65.zone>.

Most work in this repository is **prose writing and diagramming**, not code. When a chapter is marked "under construction," agents are expected to expand it into a complete, publishable chapter in the house style described below, grounded in the reference sources listed in [Sources of Truth](#sources-of-truth).

This repository is **not** the place to modify firmware, emulator, schematic, or example code — those live in child repositories mounted as git submodules.

---

## Repository Layout

- [`book/`](./) — the book (Sphinx project)
  - [`book/index.md`](index.md) — root page; defines the top-level `{toctree}` (`part_*` + `A/*`)
  - [`book/part_1.md`](part_1.md) + [`book/1/`](1/) — Part 1: hardware and architecture
  - [`book/part_2.md`](part_2.md) + [`book/2/`](2/) — Part 2: 65816 assembly programming
  - [`book/A/`](A/) — Appendices (memory map, glossary, 65816 migration, systems comparison, cheat sheet)
  - [`book/conf.py`](conf.py) — Sphinx configuration
  - [`book/requirements.txt`](requirements.txt) — Python packages
  - [`book/CMakeLists.txt`](CMakeLists.txt) — build glue (Sphinx)
  - [`book/_static/`](_static/) — images, logo, custom CSS
- [`CMakeLists.txt`](../CMakeLists.txt) — root project; delegates to `book/`
- Submodules (listed in [`.gitmodules`](../.gitmodules)): [`firmware/`](../firmware/), [`schematic/`](../schematic/), [`emulator/`](../emulator/), [`examples/`](../examples/)
- [`reference/`](../reference/) — **local directory** (not a submodule) of third-party datasheets and background HTML/PDFs

## File & Chapter Naming Conventions

Follow the existing numbering scheme so the `{toctree} :glob:` directives pick new chapters up automatically:

| Location | Pattern | Examples |
| --- | --- | --- |
| [`book/1/`](1/) | `N_topic.md` (single digit) | `4_graphics.md`, `6_io.md` |
| [`book/2/`](2/) | `NN_topic.md` (zero-padded) | `08_assembly.md`, `11_graphics.md` |
| [`book/A/`](A/) | `L_topic.md` (capital letter) | `A_memory_map.md`, `B_glossary.md` |

Inside each file, the top heading is `# Chapter N: Title` (Part 1/2) or `# Appendix L: Title` (appendices). Stubs open with a `{note}` admonition stating that the chapter is under construction.

## Building the Book

Build from the **repository root** (the root `CMakeLists.txt` adds `book/` as a subdirectory):

```sh
cmake -S . -B build
cmake --build build
```

Generated HTML lives at `build/book/sphinx/` (Sphinx is invoked with `-b dirhtml`).

**Prerequisites:** Sphinx and the Python packages in [`book/requirements.txt`](requirements.txt) (`myst-parser`, `furo`, `sphinx-sitemap`, `sphinx-design`, `sphinx-inline-tabs`, `sphinx-copybutton`, `sphinxext-opengraph`).

The build runs Sphinx with `-W`, so **any warning is a build error** — there is nothing to triage and no baseline to compare against. If `cmake --build build` fails, read the warning: it is a broken link, a missing toctree entry or an unknown directive, and it is yours. Do not silence one with `suppress_warnings`; fix the cause, or say so if you believe the warning is wrong.

## Writing Style & Conventions

- All content is **[MyST Markdown](https://myst-parser.readthedocs.io/)** (`.md`). Use standard Markdown plus MyST directives where helpful.
- Enabled MyST/Sphinx extensions you may freely use:
  - `colon_fence` — `:::{note}` / `:::{warning}` admonition syntax
  - heading anchors are generated down to level 4 (`myst_heading_anchors`), so you can link to a specific section as `[text](../1/4_graphics.md#some-heading)`
  - `sphinx_design` — grids, cards, tabs, dropdowns
  - `sphinx_inline_tabs` — language/platform tabs
  - `sphinx_copybutton` — automatic copy buttons on code blocks
  - `sphinx.ext.mathjax` — LaTeX-style math via `$...$` and `$$...$$`
- Prefer **prose with bolded key terms** for first-time concepts (e.g. `**CGIA**`, `**Display List**`). Follow the voice in [`book/1/1_introduction.md`](1/1_introduction.md) and [`book/1/4_graphics.md`](1/4_graphics.md) — declarative, educational, moderately formal.
- Use tables for register maps, bit layouts, opcode listings (see [`book/A/E_cheat_sheet.md`](A/E_cheat_sheet.md)).
- Fenced assembly code blocks use the language tag ```` ```asm ````; C code uses ```` ```c ````.
- Cross-reference other chapters using **relative Markdown links**, e.g. `[Chapter 4](../1/4_graphics.md)`. To target a section, append the heading's slug. **Watch out for em dashes**: MyST drops the `—` but keeps the spaces on either side, so `#### MODE0 and MODE1 — Paletted Modes` is `#mode0-and-mode1--paletted-modes` — two dashes, even though the id in the rendered HTML has one. If a heading link warns, that is usually why. Do **not** hyperlink into submodule content — the published site does not expose submodule source trees.
- Keep images in [`book/_static/`](_static/) and reference them with relative paths.
- Glossary terms go in [`book/A/B_glossary.md`](A/B_glossary.md); add entries when introducing new jargon.

## Adding or Expanding Content

1. Place new chapters in the matching part directory (`1/`, `2/`, `A/`) using the naming convention above; the glob `{toctree}` will include them.
2. If a chapter is a stub (just `{note}` + section headings), flesh it out by following the structure of the established chapters ([`book/1/4_graphics.md`](1/4_graphics.md) is a good example of depth and voice). Remove the "under construction" note only when the chapter is complete.
3. When new jargon appears, extend [`book/A/B_glossary.md`](A/B_glossary.md) and the [`book/A/E_cheat_sheet.md`](A/E_cheat_sheet.md) where applicable.
4. Update [`book/index.md`](index.md) / `part_*.md` **only** when adding content outside the glob patterns (new appendices follow the existing glob).
5. Rebuild and review the generated HTML before declaring a change complete.

## Sources of Truth

When researching facts about the machine, use these sources **in priority order**. Never modify submodule content from this repo, and never hyperlink from the book into a submodule path.

1. **[`firmware/`](../firmware/)** (submodule) — authoritative implementation of the X65's custom chips on RP2350 microcontrollers. The tree is organized as:
   - [`firmware/src/south/cgia/`](../firmware/src/south/cgia/) — CGIA (graphics) implementation and encode routines
   - [`firmware/src/south/`](../firmware/src/south/) — South chip: audio PIO, system, terminal, fonts
   - [`firmware/src/north/`](../firmware/src/north/) — North chip: CPU PIO, monitor, HID, API surface
   - [`firmware/src/audio/`](../firmware/src/audio/) — audio co-processor

   If the book and firmware disagree, **the book is wrong** — update the book. Paraphrase freely; do not hyperlink.
2. **[`schematic/`](../schematic/)** (submodule) — KiCad + PDF for board-level details (pinouts, bus wiring, connectors). Cite behavior; do not embed copyrighted excerpts verbatim.
3. **[`emulator/`](../emulator/)** (submodule) — C source for the X65 emulator; useful for overall architecture and CPU/bus interactions. **Generalize** — the emulator is a model, not the machine. Do not copy verbatim.
4. **[`examples/`](../examples/)** (submodule) — reference 65816 programs for Part 2. Adapt snippets into the book with surrounding narrative and updated context; do not link out to the submodule.
5. **[`reference/`](../reference/)** (local, **not** a submodule) — third-party datasheets and background articles (W65C816, YMF262, RP2350, HDMI/DVI, etc.). Distill; **never quote verbatim**. Datasheets in particular are background reading, not book content.

When two sources conflict, firmware wins. When firmware is silent, schematic wins. When both are silent, generalize from emulator, then fall back to `reference/`.

## Validation Checklist (before reporting a change as done)

- [ ] `cmake --build build` succeeds — under `-W` that already means zero Sphinx warnings.
- [ ] New/renamed chapters appear in the generated HTML under `build/book/sphinx/` and in the sidebar.
- [ ] Any factual claim about hardware or registers was cross-checked against `firmware/` source.
- [ ] No hyperlinks into submodule paths; examples are adapted, not linked.
- [ ] New jargon added to the glossary.
- [ ] Relative links between chapters resolve (no MyST "cannot find document" warnings).

## Troubleshooting

- **MyST/Sphinx parse errors**: consult the [MyST reference](https://myst-parser.readthedocs.io/). Most errors are unknown directive names or mis-nested admonitions.
- **Missing Python packages**: `pip install -r book/requirements.txt`.
- **Stale rendered pages after deleting a chapter**: removing a source file never makes a build output stale, so `rm -rf build/book/sphinx` and rebuild.
- **Submodules empty**: run `git submodule update --init` from the repository root.
