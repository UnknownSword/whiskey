#include <ui/utils/mainUi.h>
#include <clocale>

// Basic colors
Color tui_color_black   (0x000000);
Color tui_color_white   (0xffffff);
Color tui_color_gray    (0x808080);

// Primary & Accent colors
Color tui_color_red     (0xff0000);
Color tui_color_green   (0x00ff00);
Color tui_color_blue    (0x0000ff);

Color tui_color_yellow  (0xffff00);
Color tui_color_cyan    (0x00ffff);
Color tui_color_magenta (0xff00ff);

Color tui_color_purple  (0x800080);
Color tui_color_orange  (0xffa500);
Color tui_color_pink    (0xffc0cb);
Color tui_color_brown   (0xa52a2a);

Color& tui_default_fg_color = tui_color_white;
Color& tui_default_bg_color = tui_color_black;

// ─────────────────────────────────────────────
// Gray Scale (10 shades)
// ─────────────────────────────────────────────
Color tui_color_gray_scale[10] = {
    Color(0xFFFFFF),  // 255
    Color(0xE0E0E0),  // 224
    Color(0xC2C2C2),  // 194
    Color(0xA3A3A3),  // 163
    Color(0x858585),  // 133
    Color(0x666666),  // 102
    Color(0x484848),  // 72
    Color(0x292929),  // 41
    Color(0x0F0F0F),  // 15
    Color(0x000000)   // 0
};




bool tui_init(){
	setlocale(LC_ALL, "");
	initscr();
	bool ret = 1;
	if(!has_colors())
		ret = 0;
	start_color();
	curs_set(0);
	noecho();
	keypad(stdscr, TRUE);
	curs_set(0);
	return ret;
}
void tui_end(){
	endwin();
}

