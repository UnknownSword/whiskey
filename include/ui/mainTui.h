#pragma once

#include <ui/utils/mainUi.h>
#include <ui/utils/color.h>
#include <cacheData.h>
#include <commandLine.h>
#include <string>
#include <vector>
#include <map>

enum class MainView {
	Login,
	Bag,
	Password,
	About,
	UserInfo,
	Game,
	MinerMenu,
};

class MainTui {
public:
	MainTui();
	~MainTui();

	void run();

private:
	WINDOW* title_bar;
	WINDOW* sidebar;
	WINDOW* content;
	WINDOW* status_bar;

	MainView current_view;
	int menu_selected;
	bool running;
	bool logged_in;

	// Navigation state
	bool sidebar_active;
	int bag_selected;
	bool show_help;

	// CLI panel state
	bool cli_active;
	std::string cli_input;
	int cli_cursor;
	std::vector<std::pair<std::string, std::string>> cli_history; // command -> output
	int cli_scroll;
	bool cli_editing; // true when typing in CLI

	std::string username;
	int coins;
	CacheData bag;

	// Sidebar items
	struct MenuItem {
		std::string label;
		MainView view;
	};
	std::vector<MenuItem> menu_items;

	// Bag items (indexed for selection)
	struct BagItem {
		std::string name;
		int count;
	};
	std::vector<BagItem> bag_items;

	// Item art + description paths loaded from cache/items/*.wh
	std::map<std::string, std::string> item_arts;
	std::map<std::string, std::string> item_descriptions;

	// Login input
	std::string login_user;
	std::string login_pass;
	int login_field;
	bool login_editing;

	// Password input
	std::string new_pass;
	bool pass_editing;

	// Miner menu state
	int miner_menu_selected;
	std::string miner_status_msg;

	// Colors
	ScreenColor title_color;
	ScreenColor sidebar_color;
	ScreenColor sidebar_selected_color;
	ScreenColor sidebar_normal_color;
	ScreenColor content_color;
	ScreenColor status_color;
	ScreenColor input_color;
	ScreenColor input_active_color;
	ScreenColor label_color;
	ScreenColor coin_color;
	ScreenColor item_color;
	ScreenColor highlight_color;
	ScreenColor art_color;
	ScreenColor bg_color;
	ScreenColor help_color;
	ScreenColor help_border_color;

	void init_colors();

	void setup_windows();
	void destroy_windows();

	void build_menu();
	void build_bag_items();
	void load_item_defs();

	MainView view_for_menu(int idx);

	void draw_title();
	void draw_sidebar();
	void draw_content();
	void draw_status();
	void draw_all();
	void draw_help_overlay();

	void draw_login_view();
	void draw_bag_view();
	void draw_password_view();
	void draw_about_view();
	void draw_userinfo_view();
	void draw_shop_preview();
	void draw_game_preview();
	void draw_miner_menu();
	void draw_action_preview(const std::string& title, const std::string& desc);
	void draw_cli_panel();

	int load_art(const std::string& art_path, WINDOW* win, int start_y, int max_lines, int x_offset = 3);

	void handle_input(int key);
	void handle_sidebar_keys(int key);
	void handle_content_keys(int key);
	void handle_login_keys(int key);
	void handle_password_keys(int key);
	void handle_cli_keys(int key);
	void handle_miner_menu_keys(int key);

	void cli_execute(const std::string& line);

	void do_login();
	void do_change_password();
	void open_shop();
	void do_logout();

	void miner_collect();
	void miner_buy_worker();
	void miner_sell_worker();
	void miner_level_up();
	void load_miner_data();

	void load_bag();
	void save_bag();
};
