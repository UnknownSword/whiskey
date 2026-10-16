#include <ui/minerTui.h>
#include <cacheData.h>
#include <globals.h>
#include <fstream>
#include <sstream>
#include <cstring>
#include <filesystem>
#include <algorithm>
#include <ncurses.h>
#include <cmath>

MinerTui::MinerTui()
	: left_panel(nullptr), right_panel(nullptr),
	  status_bar(nullptr), title_bar(nullptr),
	  player_coins(0), gold_in_bag(0), running(false), show_help(false),
	  level(1), gold_per_minute(2), max_workers(5), workers(0),
	  max_gold(100), last_collect_time(0), current_gold(0.0),
	  title_color(tui_color_yellow, tui_color_black, A_BOLD | A_UNDERLINE),
	  box_color(tui_color_purple, tui_color_black),
	  selected_color(tui_color_green, tui_color_gray_scale[7], A_BOLD),
	  normal_color(tui_color_gray_scale[2], tui_color_black),
	  art_color(tui_color_cyan, tui_color_black),
	  status_color(tui_color_white, tui_color_gray_scale[8], A_BOLD),
	  price_color(tui_color_yellow, tui_color_black, A_BOLD),
	  coin_color(tui_color_green, tui_color_black, A_BOLD),
	  gold_color(tui_color_yellow, tui_color_black, A_BOLD),
	  bg_color(tui_color_white, tui_color_black),
	  help_color(tui_color_white, tui_color_gray_scale[6], A_BOLD),
	  help_border_color(tui_color_yellow, tui_color_gray_scale[6])
{
}

MinerTui::~MinerTui() {
	destroy_windows();
}

void MinerTui::setup_windows() {
	int max_y, max_x;
	getmaxyx(stdscr, max_y, max_x);

	int title_h = 3;
	int status_h = 3;
	int content_h = max_y - title_h - status_h;
	int left_w = 40;
	int right_w = max_x - left_w;

	title_bar = newwin(title_h, max_x, 0, 0);
	left_panel = newwin(content_h, left_w, title_h, 0);
	right_panel = newwin(content_h, right_w, title_h, left_w);
	status_bar = newwin(status_h, max_x, title_h + content_h, 0);

	keypad(left_panel, TRUE);
	keypad(stdscr, TRUE);

	attr_t bg_attr = bg_color.get();
	wbkgd(title_bar, bg_attr);
	wbkgd(left_panel, bg_attr);
	wbkgd(right_panel, bg_attr);
	wbkgd(status_bar, bg_attr);
}

void MinerTui::destroy_windows() {
	if (title_bar) { delwin(title_bar); title_bar = nullptr; }
	if (left_panel) { delwin(left_panel); left_panel = nullptr; }
	if (right_panel) { delwin(right_panel); right_panel = nullptr; }
	if (status_bar) { delwin(status_bar); status_bar = nullptr; }
}

void MinerTui::load_miner_data() {
	std::string user_path = "cache/" + current_user + "/data.wh";
	CacheData userdata;
	userdata.cache_get(user_path);

	player_coins = userdata.contains_value("coins") ? userdata.get_value<int>("coins") : 100;

	if (userdata.contains_child("gold")) {
		CacheData& g = userdata.get_child("gold");
		gold_in_bag = g.contains_value("count") ? g.get_value<int>("count") : 0;
	} else {
		gold_in_bag = 0;
	}

	std::string miner_path = "cache/" + current_user + "/miner.wh";
	CacheData miner;
	miner.cache_get(miner_path);

	if (!miner.values.empty()) {
		level = miner.contains_value("level") ? miner.get_value<int>("level") : 1;
		gold_per_minute = miner.contains_value("gold_per_minute") ? miner.get_value<int>("gold_per_minute") : 2;
		max_workers = miner.contains_value("max_workers") ? miner.get_value<int>("max_workers") : 5;
		workers = miner.contains_value("workers") ? miner.get_value<int>("workers") : 1;
		max_gold = miner.contains_value("max_gold") ? miner.get_value<int>("max_gold") : 100;
		last_collect_time = miner.contains_value("last_collect_time") ? (time_t)miner.get_value<long long>("last_collect_time") : time(nullptr);
	} else {
		level = 1;
		gold_per_minute = 2;
		max_workers = 5;
		workers = 1;
		max_gold = 100;
		last_collect_time = time(nullptr);
	}

	calc_current_gold();
}

void MinerTui::save_miner_data() {
	std::string user_path = "cache/" + current_user + "/data.wh";
	CacheData userdata;
	userdata.cache_get(user_path);

	userdata.set_value("coins", player_coins);

	if (gold_in_bag <= 0) {
		userdata.childs.erase("gold");
	} else {
		userdata.childs["gold"].set_value("count", gold_in_bag);
	}

	userdata.cache_put(user_path);

	std::string miner_path = "cache/" + current_user + "/miner.wh";
	CacheData miner;
	miner.set_value("level", level);
	miner.set_value("gold_per_minute", gold_per_minute);
	miner.set_value("max_workers", max_workers);
	miner.set_value("workers", workers);
	miner.set_value("max_gold", max_gold);
	miner.set_value("last_collect_time", (long long)last_collect_time);

	miner.cache_put(miner_path);
}

void MinerTui::calc_current_gold() {
	if (workers <= 0 || gold_per_minute <= 0) {
		current_gold = 0.0;
		return;
	}

	time_t now = time(nullptr);
	double elapsed = difftime(now, last_collect_time);
	double minutes = elapsed / 60.0;
	current_gold = minutes * gold_per_minute * workers;
	if (current_gold < 0) current_gold = 0;
	if (current_gold > max_gold)
		current_gold = (double)max_gold;
}

int MinerTui::worker_cost() {
	return (int)(pow(workers + 1, 1.2) * 50) + 3;
}

int MinerTui::level_up_cost() {
	return (int)(pow(1.2, level) * 5) + 3;
}

// ============================================================
// Drawing
// ============================================================

void MinerTui::draw_title() {
	wclear(title_bar);
	int max_x = getmaxx(title_bar);

	std::string title = "=== MINER ===";
	int tx = (max_x - title.size()) / 2;

	title_color.enable(title_bar);
	mvwprintw(title_bar, 1, tx, "%s", title.c_str());
	title_color.disable(title_bar);

	box_color.enable(title_bar);
	box(title_bar, 0, 0);
	box_color.disable(title_bar);

	wnoutrefresh(title_bar);
}

void MinerTui::draw_stats_panel() {
	wclear(left_panel);
	int max_y = getmaxy(left_panel);
	int max_x = getmaxx(left_panel);

	int row = 1;

	// Title
	selected_color.enable(left_panel);
	mvwprintw(left_panel, row, 2, "--- Miner Stats ---");
	selected_color.disable(left_panel);
	row += 2;

	// Level
	price_color.enable(left_panel);
	mvwprintw(left_panel, row, 2, "Level:        %d / 100", level);
	price_color.disable(left_panel);
	row++;

	// Gold per minute
	gold_color.enable(left_panel);
	mvwprintw(left_panel, row, 2, "Gold/min:     %d", gold_per_minute);
	gold_color.disable(left_panel);
	row++;

	// Total income
	int total_income = gold_per_minute * workers;
	gold_color.enable(left_panel);
	mvwprintw(left_panel, row, 2, "Total income: %d / min", total_income);
	gold_color.disable(left_panel);
	row += 2;

	// Workers
	coin_color.enable(left_panel);
	mvwprintw(left_panel, row, 2, "Workers:      %d / %d", workers, max_workers);
	coin_color.disable(left_panel);
	row++;

	// Worker cost
	normal_color.enable(left_panel);
	mvwprintw(left_panel, row, 2, "Worker cost:  %d coins", worker_cost());
	normal_color.disable(left_panel);
	row += 2;

	// Max gold
	gold_color.enable(left_panel);
	mvwprintw(left_panel, row, 2, "Max gold:     %d", max_gold);
	gold_color.disable(left_panel);
	row++;

	// Current gold (available to collect)
	int display_gold = (int)current_gold;
	gold_color.enable(left_panel);
	mvwprintw(left_panel, row, 2, "Available:    %d gold", display_gold);
	gold_color.disable(left_panel);
	row++;

	// Level up cost
	normal_color.enable(left_panel);
	mvwprintw(left_panel, row, 2, "Level cost:   %d gold", level_up_cost());
	normal_color.disable(left_panel);
	row += 2;

	// Separator
	status_color.enable(left_panel);
	for (int x = 1; x < max_x - 1; x++)
		mvwaddch(left_panel, row, x, ACS_HLINE);
	status_color.disable(left_panel);
	row++;

	// Coins
	coin_color.enable(left_panel);
	mvwprintw(left_panel, row, 2, "Coins: %d", player_coins);
	coin_color.disable(left_panel);
	row++;

	// Gold in bag
	gold_color.enable(left_panel);
	mvwprintw(left_panel, row, 2, "Gold in bag:  %d", gold_in_bag);
	gold_color.disable(left_panel);
	row += 2;

	// Actions
	selected_color.enable(left_panel);
	mvwprintw(left_panel, row, 2, "--- Actions ---");
	selected_color.disable(left_panel);
	row++;

	normal_color.enable(left_panel);
	mvwprintw(left_panel, row, 2, "[c] Collect gold");
	row++;
	mvwprintw(left_panel, row, 2, "[w] Buy worker  (%d coins)", worker_cost());
	row++;
	mvwprintw(left_panel, row, 2, "[W] Sell worker (+%d coins)", worker_cost() / 2);
	row++;
	mvwprintw(left_panel, row, 2, "[l] Level up    (%d gold)", level_up_cost());
	normal_color.disable(left_panel);

	box_color.enable(left_panel);
	box(left_panel, 0, 0);
	box_color.disable(left_panel);

	wnoutrefresh(left_panel);
}

void MinerTui::draw_art_panel() {
	wclear(right_panel);
	int max_y = getmaxy(right_panel);
	int max_x = getmaxx(right_panel);

	// Title
	title_color.enable(right_panel);
	std::string title = "--- Gold Mine ---";
	int tx = (max_x - title.size()) / 2;
	mvwprintw(right_panel, 1, tx > 0 ? tx : 1, "%s", title.c_str());
	title_color.disable(right_panel);

	// Load and draw miner art
	std::ifstream art_file("assets/arts/items/miner.art");
	if (art_file.is_open()) {
		art_color.enable(right_panel);
		int y = 3;
		std::string line;
		int art_max = max_y / 2 - 2;
		while (std::getline(art_file, line) && y < 3 + art_max) {
			int panel_w = max_x - 6;
			if ((int)line.size() > panel_w)
				line = line.substr(0, panel_w);
			mvwprintw(right_panel, y, 3, "%s", line.c_str());
			y++;
		}
		art_color.disable(right_panel);

		// Description
		std::ifstream desc_file("assets/descriptions/miner.txt");
		if (desc_file.is_open()) {
			status_color.enable(right_panel);
			int dy = y + 2;
			std::string dline;
			int panel_w = max_x - 6;
			while (std::getline(desc_file, dline) && dy < max_y - 4) {
				if ((int)dline.size() > panel_w)
					dline = dline.substr(0, panel_w);
				mvwprintw(right_panel, dy, 3, "%s", dline.c_str());
				dy++;
			}
			status_color.disable(right_panel);
		}
	} else {
		art_color.enable(right_panel);
		mvwprintw(right_panel, max_y / 2, (max_x - 15) / 2, "[art not found]");
		art_color.disable(right_panel);
	}

	// Gold production animation at bottom
	if (workers > 0) {
		int bottom_y = max_y - 4;
		int total_income = gold_per_minute * workers;
		gold_color.enable(right_panel);
		std::string prod = ">> Producing " + std::to_string(total_income) + " gold/min <<";
		int px = (max_x - (int)prod.size()) / 2;
		mvwprintw(right_panel, bottom_y, px > 0 ? px : 3, "%s", prod.c_str());
		gold_color.disable(right_panel);
	} else {
		int bottom_y = max_y - 4;
		normal_color.enable(right_panel);
		std::string msg = "-- No workers! Buy one to start --";
		int mx = (max_x - (int)msg.size()) / 2;
		mvwprintw(right_panel, bottom_y, mx > 0 ? mx : 3, "%s", msg.c_str());
		normal_color.disable(right_panel);
	}

	box_color.enable(right_panel);
	box(right_panel, 0, 0);
	box_color.disable(right_panel);

	wnoutrefresh(right_panel);
}

void MinerTui::draw_all() {
	draw_title();
	draw_stats_panel();
	draw_art_panel();
	if (show_help)
		draw_help_overlay();
	doupdate();
}

void MinerTui::draw_help_overlay() {
	int max_y, max_x;
	getmaxyx(stdscr, max_y, max_x);

	int h = 16;
	int w = 48;
	int y = (max_y - h) / 2;
	int x = (max_x - w) / 2;

	WINDOW* win = newwin(h, w, y, x);

	help_border_color.enable(win);
	box(win, 0, 0);
	help_border_color.disable(win);

	help_color.enable(win);

	mvwprintw(win, 1, (w - 11) / 2, "=== HELP ===");

	int row = 3;
	auto line = [&](const std::string& key, const std::string& desc) {
		mvwprintw(win, row, 3, "%-14s %s", key.c_str(), desc.c_str());
		row++;
	};

	mvwprintw(win, row, 3, "-- Miner --");
	row++;
	line("c", "Collect available gold");
	line("w", "Buy a worker");
	line("W", "Sell a worker");
	line("l", "Level up miner");
	line("?", "Show this help");
	line("q", "Exit miner");

	row++;
	mvwprintw(win, row, 3, "Press any key to close");

	help_color.disable(win);
	wnoutrefresh(win);
	doupdate();

	wgetch(win);
	delwin(win);
}

// ============================================================
// Actions
// ============================================================

void MinerTui::do_collect() {
	calc_current_gold();
	int collect = (int)current_gold;
	if (collect <= 0) return;
	if (collect > max_gold) collect = max_gold;

	gold_in_bag += collect;
	last_collect_time = time(nullptr);
	current_gold = 0.0;
	save_miner_data();
}

void MinerTui::do_buy_worker() {
	if (workers >= max_workers) return;
	int cost = worker_cost();
	if (cost > player_coins) return;

	player_coins -= cost;
	workers++;
	save_miner_data();
}

void MinerTui::do_sell_worker() {
	if (workers <= 0) return;
	int refund = worker_cost() / 2;
	workers--;
	player_coins += refund;
	save_miner_data();
}

void MinerTui::do_level_up() {
	if (level >= 100) return;
	int cost = level_up_cost();
	if (cost > gold_in_bag) return;

	gold_in_bag -= cost;
	level++;
	gold_per_minute = (int)((gold_per_minute + 5) * 1.2);
	if (gold_per_minute < 1) gold_per_minute = 1;
	max_workers += 5;
	max_gold += 50;
	save_miner_data();
}

// ============================================================
// Input
// ============================================================

void MinerTui::handle_input(int key) {
	if (show_help) {
		show_help = false;
		return;
	}

	switch (key) {
		case 'c':
			do_collect();
			break;
		case 'w':
			do_buy_worker();
			break;
		case 'W':
			do_sell_worker();
			break;
		case 'l':
			do_level_up();
			break;
		case 'q':
		case 'Q':
			running = false;
			break;
		case '?':
			show_help = true;
			break;
	}
}

// ============================================================
// Run
// ============================================================

void MinerTui::run(const std::string& username) {
	current_user = username;
	load_miner_data();

	setup_windows();
	running = true;
	draw_all();

	while (running) {
		// Recalc gold on each tick for live update
		calc_current_gold();
		draw_all();
		int key = wgetch(left_panel);
		handle_input(key);
	}

	save_miner_data();
	destroy_windows();
}
