# X65 emulator

## Preparation

This project makes use of git submodules, so you need to `git clone --recursive`.

## Building

[`zig`][1] v. 0.12 required.

    zig build

[1]: https://ziglang.org

## Running

Without UI

    zig build run

With UI

    zig build run -Dui

## IDE

Build `compile_commands.json` required by `clangd`.

    zig build cdb

This is required only first time, or after adding new files to build.
