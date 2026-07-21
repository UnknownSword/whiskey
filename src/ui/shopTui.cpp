#include <ui/shopTui.h>
#include <cacheData.h>
#include <fstream>
#include <sstream>
#include <cstring>
#include <filesystem>
#include <algorithm>
#include <ncurses.h>

static std::string strip_quotes(const std::string& s) {
	if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
		return s.substr(1, s.size() - 2);
	return s;
}

ShopTui::ShopTui()
	: left_panel(nullptr), right_panel(nullptr),
	  status_bar(nullptr), title_bar(nullptr),
	  selected_index(0), scroll_offset(0), running(false),
	  player_coins(0), show_help(false),
	  title_color(tui_color_yellow, tui_color_black, A_BOLD | A_UNDERLINE),
	  box_color(tui_color_purple, tui_color_black),
	  selected_color(tui_color_green, tui_color_gray_scale[7], A_BOLD),
	  normal_color(tui_color_gray_scale[2], tui_color_black),
	  art_color(tui_color_cyan, tui_color_black),
	  status_color(tui_color_white, tui_color_gray_scale[8], A_BOLD),
	  price_color(tui_color_yellow, tui_color_black, A_BOLD),
	  coin_color(tui_color_green, tui_color_black, A_BOLD),
	  bg_color(tui_color_white, tui_color_black),
	  help_color(tui_color_white, tui_color_gray_scale[6], A_BOLD),
	  help_border_color(tui_color_yellow, tui_color_gray_scale[6])
{
}

ShopTui::~ShopTui() {
	destroy_windows();
}

void ShopTui::setup_windows() {
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

void ShopTui::destroy_windows() {
	if (title_bar) { delwin(title_bar); title_bar = nullptr; }
	if (left_panel) { delwin(left_panel); left_panel = nullptr; }
	if (right_panel) { delwin(right_panel); right_panel = nullptr; }
	if (status_bar) { delwin(status_bar); status_bar = nullptr; }
}

void ShopTui::load_items() {
	items.clear();

	// Load item definitions from .wh files
	CacheData items_def;
	std::filesystem::path items_dir("cache/items");
	if (!std::filesystem::exists(items_dir) || !std::filesystem::is_directory(items_dir))
		return;

	for (auto& entry : std::filesystem::recursive_directory_iterator(items_dir)) {
		if (entry.is_regular_file() && entry.path().extension() == ".wh") {
			CacheData file_items;
			file_items.cache_get(entry.path().string());
			for (auto& [name, def] : file_items.childs) {
				ShopItem item;
				item.name = name;
				item.buy_price = 0;
				item.sell_price = 0;
				item.owned_count = 0;

				if (def.contains_value("art"))
					item.art_path = strip_quotes(def.get_value<std::string>("art"));
				if (def.contains_value("description"))
					item.description_path = strip_quotes(def.get_value<std::string>("description"));
				if (def.contains_value("name-color"))
					item.name_color = strip_quotes(def.get_value<std::string>("name-color"));

				// Get prices from shop cache
				items.push_back(item);
			}
		}
	}

	// Load shop prices
	CacheData shop_data;
	std::string shop_path = "cache/shop/wh";
	shop_data.cache_get(shop_path);

	for (auto& item : items) {
		if (shop_data.contains_child(item.name)) {
			CacheData& shop_item = shop_data.get_child(item.name);
			if (shop_item.contains_value("bye"))
				item.buy_price = shop_item.get_value<int>("bye");
			if (shop_item.contains_value("sell"))
				item.sell_price = shop_item.get_value<int>("sell");
		}
	}

	// Load player bag
	CacheData bag_data;
	std::string bag_path = "cache/" + current_user + "/data.wh";
	bag_data.cache_get(bag_path);

	if (bag_data.contains_value("coins"))
		player_coins = bag_data.get_value<int>("coins");

	for (auto& item : items) {
		if (bag_data.contains_child(item.name)) {
			CacheData& bag_item = bag_data.get_child(item.name);
			if (bag_item.contains_value("count"))
				item.owned_count = bag_item.get_value<int>("count");
		}
	}

	// Sort by name
	std::sort(items.begin(), items.end(), [](const ShopItem& a, const ShopItem& b) {
		return a.name < b.name;
	});
}

int ShopTui::load_art(const std::string& art_path, WINDOW* win, int start_y, int max_lines) {
	std::ifstream file(art_path);
	if (!file.is_open()) {
		mvwprintw(win, start_y, 2, "[art not found]");
		return start_y + 1;
	}

	std::string line;
	int y = start_y;
	int max_x = getmaxx(win) - 4;

	while (std::getline(file, line) && y < start_y + max_lines) {
		if ((int)line.size() > max_x)
			line = line.substr(0, max_x);
		mvwprintw(win, y, 3, "%s", line.c_str());
		y++;
	}
	return y;
}

void ShopTui::draw_title() {
	wclear(title_bar);
	int max_x = getmaxx(title_bar);

	std::string title = "=== WHISKEY SHOP ===";
	int tx = (max_x - title.size()) / 2;

	title_color.enable(title_bar);
	mvwprintw(title_bar, 1, tx, "%s", title.c_str());
	title_color.disable(title_bar);

	box_color.enable(title_bar);
	box(title_bar, 0, 0);
	box_color.disable(title_bar);

	wnoutrefresh(title_bar);
}

void ShopTui::draw_item_list() {
	wclear(left_panel);
	int max_y = getmaxy(left_panel);
	int max_x = getmaxx(left_panel);

	// Header
	selected_color.enable(left_panel);
	mvwprintw(left_panel, 0, 1, "%-*s", max_x - 2, "  ITEM             BUY    SELL  OWN");
	selected_color.disable(left_panel);

	// Items
	int visible = max_y - 2;
	int total = (int)items.size();

	if (selected_index < scroll_offset)
		scroll_offset = selected_index;
	if (selected_index >= scroll_offset + visible)
		scroll_offset = selected_index - visible + 1;
	if (scroll_offset < 0) scroll_offset = 0;
	if (scroll_offset > total - visible && total > visible)
		scroll_offset = total - visible;

	for (int i = 0; i < visible && (scroll_offset + i) < total; i++) {
		int idx = scroll_offset + i;
		auto& item = items[idx];

		bool is_selected = (idx == selected_index);
		ScreenColor& col = is_selected ? selected_color : normal_color;

		col.enable(left_panel);

		char line[256];
		snprintf(line, sizeof(line), "%c %-16s %6d %6d  x%d",
			is_selected ? '>' : ' ',
			item.name.c_str(),
			item.buy_price,
			item.sell_price,
			item.owned_count);

		// Pad to fill width
		int len = strlen(line);
		if (len < max_x - 2) {
			memset(line + len, ' ', max_x - 2 - len);
			line[max_x - 2] = '\0';
		}

		mvwprintw(left_panel, i + 1, 0, "%s", line);
		col.disable(left_panel);
	}

	box_color.enable(left_panel);
	box(left_panel, 0, 0);
	box_color.disable(left_panel);

	wnoutrefresh(left_panel);
}

void ShopTui::draw_art_panel() {
	wclear(right_panel);
	int max_y = getmaxy(right_panel);
	int max_x = getmaxx(right_panel);

	if (items.empty()) {
		art_color.enable(right_panel);
		mvwprintw(right_panel, max_y / 2, (max_x - 15) / 2, "[ NO ITEMS ]");
		art_color.disable(right_panel);
		box_color.enable(right_panel);
		box(right_panel, 0, 0);
		box_color.disable(right_panel);
		wnoutrefresh(right_panel);
		return;
	}

	auto& item = items[selected_index];

	// Item name at top
	title_color.enable(right_panel);
	std::string name_line = "--- " + item.name + " ---";
	int nx = (max_x - name_line.size()) / 2;
	mvwprintw(right_panel, 1, nx > 0 ? nx : 1, "%s", name_line.c_str());
	title_color.disable(right_panel);

	// Draw ASCII art
	int art_end = 3;
	if (!item.art_path.empty()) {
		art_end = load_art(item.art_path, right_panel, 3, max_y / 2);
	}

	// Draw description below art
	if (!item.description_path.empty()) {
		std::ifstream desc_file(item.description_path);
		if (desc_file.is_open()) {
			status_color.enable(right_panel);
			int y = art_end + 2;
			std::string dline;
			int panel_w = max_x - 6;
			while (std::getline(desc_file, dline) && y < max_y - 8) {
				if ((int)dline.size() > panel_w)
					dline = dline.substr(0, panel_w);
				mvwprintw(right_panel, y, 3, "%s", dline.c_str());
				y++;
			}
			status_color.disable(right_panel);
		}
	}

	// Prices at bottom
	int bottom_y = max_y - 5;
	price_color.enable(right_panel);
	mvwprintw(right_panel, bottom_y, 3, "Buy:  %d coins", item.buy_price);
	mvwprintw(right_panel, bottom_y + 1, 3, "Sell: %d coins", item.sell_price);
	price_color.disable(right_panel);

	coin_color.enable(right_panel);
	mvwprintw(right_panel, bottom_y + 2, 3, "Your coins: %d", player_coins);
	mvwprintw(right_panel, bottom_y + 3, 3, "You own:    x%d", item.owned_count);
	coin_color.disable(right_panel);

	box_color.enable(right_panel);
	box(right_panel, 0, 0);
	box_color.disable(right_panel);

	wnoutrefresh(right_panel);
}

void ShopTui::refresh_status() {
	wclear(status_bar);
	int max_x = getmaxx(status_bar);

	status_color.enable(status_bar);

	std::string help_text = "[UP/DOWN] Navigate  [b] Buy  [s] Sell  [?] Help  [q] Exit";
	int hx = (max_x - help_text.size()) / 2;
	mvwprintw(status_bar, 0, hx > 0 ? hx : 0, "%s", help_text.c_str());

	char coin_str[64];
	snprintf(coin_str, sizeof(coin_str), "Coins: %d", player_coins);
	mvwprintw(status_bar, 1, 2, "%s", coin_str);

	if (!items.empty()) {
		auto& item = items[selected_index];
		mvwprintw(status_bar, 1, max_x - 30, "Selected: %s", item.name.c_str());
	}

	status_color.disable(status_bar);

	box_color.enable(status_bar);
	box(status_bar, 0, 0);
	box_color.disable(status_bar);

	wnoutrefresh(status_bar);
}

void ShopTui::draw_all() {
	draw_title();
	draw_item_list();
	draw_art_panel();
	refresh_status();
	if (show_help)
		draw_help_overlay();
	doupdate();
}

void ShopTui::buy_item(int count) {
	if (items.empty()) return;
	auto& item = items[selected_index];

	int total = item.buy_price * count;
	if (total > player_coins) {
		// Flash not enough
		return;
	}

	player_coins -= total;
	item.owned_count += count;

	// Update bag data
	CacheData bag_data;
	std::string bag_path = "cache/" + current_user + "/data.wh";
	bag_data.cache_get(bag_path);
	bag_data.set_value("coins", player_coins);

	auto& bag_item = bag_data.childs[item.name];
	bag_item.set_value("count", item.owned_count);

	bag_data.cache_put(bag_path);
}

void ShopTui::sell_item(int count) {
	if (items.empty()) return;
	auto& item = items[selected_index];

	if (item.owned_count < count) return;

	int total = item.sell_price * count;
	player_coins += total;
	item.owned_count -= count;

	// Update bag data
	CacheData bag_data;
	std::string bag_path = "cache/" + current_user + "/data.wh";
	bag_data.cache_get(bag_path);
	bag_data.set_value("coins", player_coins);

	if (item.owned_count <= 0) {
		bag_data.childs.erase(item.name);
	} else {
		bag_data.childs[item.name].set_value("count", item.owned_count);
	}

	bag_data.cache_put(bag_path);
}

void ShopTui::handle_input(int key) {
	if (show_help) {
		show_help = false;
		return;
	}

	switch (key) {
		case KEY_UP:
		case 'k':
			if (!items.empty()) {
				selected_index = (selected_index + items.size() - 1) % items.size();
			}
			break;
		case KEY_DOWN:
		case 'j':
			if (!items.empty()) {
				selected_index = (selected_index + 1) % items.size();
			}
			break;
		case 'b':
			buy_item(1);
			break;
		case 'B':
			buy_item(5);
			break;
		case 's':
			sell_item(1);
			break;
		case 'S':
			sell_item(5);
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

void ShopTui::draw_help_overlay() {
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

	mvwprintw(win, row, 3, "-- Shop --");
	row++;
	line("UP / k", "Previous item");
	line("DOWN / j", "Next item");
	line("b", "Buy 1 of selected");
	line("B", "Buy 5 of selected");
	line("s", "Sell 1 of selected");
	line("S", "Sell 5 of selected");
	line("?", "Show this help");
	line("q", "Exit shop");

	row++;
	mvwprintw(win, row, 3, "Press any key to close");

	help_color.disable(win);
	wnoutrefresh(win);
	doupdate();

	wgetch(win);
	delwin(win);
}

void ShopTui::run(const std::string& username) {
	current_user = username;
	load_items();

	setup_windows();
	running = true;
	draw_all();

	while (running) {
		int key = wgetch(left_panel);
		handle_input(key);
		draw_all();
	}

	destroy_windows();
}
