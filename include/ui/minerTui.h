#pragma once

#include <ui/utils/mainUi.h>
#include <ui/utils/color.h>
#include <string>
#include <vector>

class MinerTui {
public:
	MinerTui();
	~MinerTui();

	void run(const std::string& username);

private:
	WINDOW* left_panel;
	WINDOW* right_panel;
	WINDOW* status_bar;
	WINDOW* title_bar;

	int player_coins;
	int gold_in_bag;
	std::string current_user;
	bool running;
	bool show_help;

	int level;
	int gold_per_minute;
	int max_workers;
	int workers;
	int max_gold;
	time_t last_collect_time;

	// Derived
	double current_gold;

	// Colors
	ScreenColor title_color;
	ScreenColor box_color;
	ScreenColor selected_color;
	ScreenColor normal_color;
	ScreenColor art_color;
	ScreenColor status_color;
	ScreenColor price_color;
	ScreenColor coin_color;
	ScreenColor gold_color;
	ScreenColor bg_color;
	ScreenColor help_color;
	ScreenColor help_border_color;

	void setup_windows();
	void destroy_windows();
	void load_miner_data();
	void save_miner_data();
	void calc_current_gold();

	void draw_title();
	void draw_stats_panel();
	void draw_art_panel();
	void draw_all();
	void draw_help_overlay();

	void handle_input(int key);
	void do_collect();
	void do_buy_worker();
	void do_sell_worker();
	void do_level_up();

	int worker_cost();
	int level_up_cost();
};
