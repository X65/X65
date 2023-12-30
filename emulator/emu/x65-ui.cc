/*
    UI implementaion for x65.c
*/
#include "chips/chips_common.h"
#include "chips/m6502.h"
#include "chips/m6522.h"
#include "chips/m6561.h"
#include "chips/kbd.h"
#include "chips/mem.h"
#include "chips/clk.h"
#include "systems/c1530.h"
#include "systems/x65.h"
#define CHIPS_UTIL_IMPL
#include "util/m6502dasm.h"
#define CHIPS_UI_IMPL
#include "util/m6502dasm.h"
#include "systems/ui_x65.h"
