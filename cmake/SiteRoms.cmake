# ROMs the web emulator on x65.zone serves out of `emu/roms/`, and which target
# in the `examples` submodule builds each one.
#
# The site's own catalogue is `x65.zone/_data/emu.yml`; this list mirrors the
# entries there whose `src:` points at X65/examples. Everything else in
# `emu/roms/` is hand-made or comes from elsewhere (MODE1_lobo_2, MODE7_mascot,
# PetsciiRobots, Dune_Chani, SMON...) and the `site` target must leave it alone.
#
# Add a row when a new example earns a place on the site. The published name is
# what emu.yml refers to, so it does not have to match the target.

# Not every example is a whole ROM. `four_byte_burger`, `mixed_modes` and `sotb`
# assemble to code only - the image, layer and sprite data is merged in afterwards
# with `xex-filter.pl` (their .asm header comments carry the exact commands), and
# the blocks come from `examples/src/cgia/data/bins.c`, which is a `gcc bins.c &&
# ./bins` hack rather than part of any build. Publishing the bare code gives a ROM
# that loads and shows nothing, so they are deliberately absent below and the
# committed ones on the site are left alone. `four_byte_burger` cannot be rebuilt
# here at all: its image comes from a `converter.ts` and a `4BB.png` that are not
# in this tree.

# <examples target>  <name published under emu/roms/>
set(SITE_ROMS
    text_modes          MODE0_text
    swboy               MODE2_SWBoy
    text_80_columns     80_columns
    raster_bars         raster_bars
    rsi                 RSI
    vbl                 VBI
    sprites             sprites
    lkhs_raster_bar     raster_bar
    font_cp             font_cp
    controller          controller
    hello_uart          hello_uart
    gpio                gpio
    bogo                bogo
    opl                 opl
    MontyOnTheRun.sid   MontyOnTheRun
)

# SID/SAP players ship as a bare player: the tune is a separate file that has to
# be loaded alongside it, which is what `xex-filter.pl` splices in. The load
# address is the one the player was assembled against - it is `LOADADDRESS` in
# the matching `examples/src/sound/*.asm`, and the header comment there spells
# out the same two xex-filter calls. Keep the two in step; nothing checks.
#
# <examples target>  <published name>  <tune file, relative to examples/>  <load address>
# (404_Error.sap is deliberately absent - the site does not serve it.)
set(SITE_TUNE_ROMS
    Driller.sid         Driller         src/sound/Driller.sid                 $0882
    Mystery_Cannon.sid  Mystery_Cannon  src/sound/Mystery_Cannon.sid          $0F82
    Pokeymania.sid      Pokeymania      src/sound/Orbtraxx2-Pokeymania.sid    $1E82
)
