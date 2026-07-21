#pragma once

#include <string>
#include <vector>
#include <ui/utils/mainUi.h>

struct TuiMenuOption{
	std::string title;
	
	void* data;
	
	// void (*)(TuiMenu*)
	void (*onClick)(void*);
	
	TuiMenuOption(std::string = "", void (*)(void*) = nullptr);
};

struct TuiMenu{
	ScreenColor title_color				= (ScreenColor){ tui_color_magenta, tui_color_gray_scale[7], A_BOLD | A_UNDERLINE};
	ScreenColor box_color				= (ScreenColor){ tui_color_purple, tui_color_gray_scale[7], A_NORMAL};
	ScreenColor selected_item_color		= (ScreenColor){ tui_color_green , tui_color_gray_scale[5], A_BOLD  };
	ScreenColor not_selected_item_color	= (ScreenColor){ tui_color_red   , tui_color_gray_scale[7], A_NORMAL};
	
	std::string selected_item_prefix = "-> ";
	std::string not_selected_item_prefix = " * ";
	
	std::string title = "";
	
	bool is_my_window = 0;
	WINDOW* window = nullptr;
	
	std::vector<TuiMenuOption> option;
	int selected_item = 0;
	
	bool isactive;
	
	TuiMenu(std::string = "", WINDOW* = nullptr);
	TuiMenu(std::string, int, int, int, int);
	~TuiMenu();
	
	void delete_window();
	void change_window(int, int, int, int);
	
	void add_option(TuiMenuOption);
	void delete_option_index(int);
	void delete_option_title(std::string);
	
	size_t option_count();
	void fix_size();
	void clear();
	
	int get_maxy();
	int get_maxx();
	int get_begy();
	int get_begx();
	
	void center(int, int, int, int);
	void center(WINDOW* = stdscr);
	
	bool is_active();
	void enable();
	void disable();
	
	void display();
	void handle_input(int);
	void run();
};

void tui_menu_option_exit_function(void* tuimenu);

const TuiMenuOption tui_menu_option_exit("Exit", tui_menu_option_exit_function);

