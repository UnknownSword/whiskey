#pragma once

#include <ui/utils/mainUi.h>
#include <ui/utils/color.h>
#include <string>
#include <vector>
#include <map>

// TUI Shop Game for Whiskey
// Displays items with ASCII art in an ncurses interface

struct ShopItem {
	std::string name;
	int buy_price;
	int sell_price;
	std::string art_path;
	std::string description_path;
	std::string name_color;
	int owned_count;
};

class ShopTui {
public:
	ShopTui();
	~ShopTui();

	void run(const std::string& username);

private:
	// Layout
	WINDOW* left_panel;
	WINDOW* right_panel;
	WINDOW* status_bar;
	WINDOW* title_bar;

	// State
	std::vector<ShopItem> items;
	int selected_index;
	int scroll_offset;
	bool running;
	int player_coins;
	std::string current_user;
	bool show_help;

	// Colors
	ScreenColor title_color;
	ScreenColor box_color;
	ScreenColor selected_color;
	ScreenColor normal_color;
	ScreenColor art_color;
	ScreenColor status_color;
	ScreenColor price_color;
	ScreenColor coin_color;
	ScreenColor bg_color;
	ScreenColor help_color;
	ScreenColor help_border_color;

	// Setup
	void setup_windows();
	void destroy_windows();
	void load_items();
	int load_art(const std::string& art_path, WINDOW* win, int start_y, int max_lines);
	void refresh_status();

	// Display
	void draw_title();
	void draw_item_list();
	void draw_art_panel();
	void draw_all();
	void draw_help_overlay();

	// Input
	void handle_input(int key);
	void buy_item(int count);
	void sell_item(int count);
};
