#pragma once

#include <ncurses.h>
#include <ui/utils/color.h>

bool tui_init();
void tui_end();

// Basic colors
extern Color tui_color_black   ;
extern Color tui_color_white   ;
extern Color tui_color_gray    ;

// Primary & Accent colors
extern Color tui_color_red     ;
extern Color tui_color_green   ;
extern Color tui_color_blue    ;

extern Color tui_color_yellow  ;
extern Color tui_color_cyan    ;
extern Color tui_color_magenta ;

extern Color tui_color_purple  ;
extern Color tui_color_orange  ;
extern Color tui_color_pink    ;
extern Color tui_color_brown   ;

extern Color& tui_default_fg_color;
extern Color& tui_default_bg_color;

// ─────────────────────────────────────────────
// Gray Scale (10 shades)
// ─────────────────────────────────────────────
extern Color tui_color_gray_scale[10] ;



