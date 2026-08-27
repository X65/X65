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

# Not every example is a whole ROM. Ones that assemble to code only and have to
# be merged with data blocks are in SITE_DATA_ROMS further down, not here -
# copying the bare code would publish a ROM that loads and shows nothing.

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

# ROMs that are code plus a picture. `bins.c` writes the data blocks (see
# ExampleData.cmake) and `xex-filter.pl` splices them onto the assembled code;
# the .asm header comment of each demo is the reference for its recipe, and this
# is the copy the build follows - keep the two in step.
#
# DATA names blocks in the example-data staging directory. RELOCATE, if set,
# gives every block of the code (and of anything in WITH) a new load address in
# a first pass, before the data is appended.
#
# `four_byte_burger` is missing on purpose: its picture is produced by a
# `converter.ts` from a `4BB.png` that are not in this tree, so the ROM on the
# site is hand-made and `site` leaves it alone.
set(SITE_DATA_ROMS
    sotb
    mixed_modes
)

set(SITE_DATA_ROM_sotb_PUBLISHED SOTB)
set(SITE_DATA_ROM_sotb_DATA      sotb_layers.xex sotb_sprite.xex)

# The code and the shared 8px font are lifted out of the memory the picture
# occupies, then the display list, the MODE7 texture and the HUD are appended.
set(SITE_DATA_ROM_mixed_modes_PUBLISHED mixed_modes)
set(SITE_DATA_ROM_mixed_modes_RELOCATE  "$B000,$FC00,$FFE0,$A000")
set(SITE_DATA_ROM_mixed_modes_WITH      ${EMU_SOURCE_DIR}/roms/parts/font_8px.xex)
set(SITE_DATA_ROM_mixed_modes_DATA      mixed_mode_dl.xex mascot_bg.xex hud_layer.xex)
