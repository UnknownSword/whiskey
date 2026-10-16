#include <ui/mainTui.h>
#include <ui/shopTui.h>
#include <ui/minerTui.h>
#include <cacheData.h>
#include <globals.h>
#include <utils/random.h>
#include <ncurses.h>
#include <cstring>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <fcntl.h>
#include <ctime>

static std::string strip_quotes(const std::string& s) {
	if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
		return s.substr(1, s.size() - 2);
	return s;
}

// ============================================================
// Constructor / Destructor
// ============================================================

MainTui::MainTui()
	: title_bar(nullptr), sidebar(nullptr), content(nullptr), status_bar(nullptr),
	  current_view(MainView::Login), menu_selected(0), running(false), logged_in(false),
	  sidebar_active(true), bag_selected(0), show_help(false),
	  cli_active(false), cli_cursor(0), cli_scroll(0), cli_editing(false),
	  coins(0), login_field(0), login_editing(false), pass_editing(false),
	  login_user_selected(0), login_creating(false),
	  miner_menu_selected(0)
{
}

MainTui::~MainTui() {
	destroy_windows();
}

// ============================================================
// Color init
// ============================================================

void MainTui::init_colors() {
	title_color         = ScreenColor(tui_color_yellow, tui_color_black, A_BOLD | A_UNDERLINE);
	sidebar_color       = ScreenColor(tui_color_purple, tui_color_black);
	sidebar_selected_color = ScreenColor(tui_color_green, tui_color_gray_scale[7], A_BOLD);
	sidebar_normal_color = ScreenColor(tui_color_gray_scale[2], tui_color_black);
	content_color       = ScreenColor(tui_color_white, tui_color_black);
	status_color        = ScreenColor(tui_color_white, tui_color_gray_scale[8], A_BOLD);
	input_color         = ScreenColor(tui_color_gray_scale[3], tui_color_gray_scale[7]);
	input_active_color  = ScreenColor(tui_color_green, tui_color_gray_scale[7], A_BOLD);
	label_color         = ScreenColor(tui_color_cyan, tui_color_black);
	coin_color          = ScreenColor(tui_color_green, tui_color_black, A_BOLD);
	item_color          = ScreenColor(tui_color_white, tui_color_black);
	highlight_color     = ScreenColor(tui_color_yellow, tui_color_black, A_BOLD);
	art_color           = ScreenColor(tui_color_cyan, tui_color_black);
	bg_color            = ScreenColor(tui_color_white, tui_color_black);
	help_color          = ScreenColor(tui_color_white, tui_color_gray_scale[6], A_BOLD);
	help_border_color   = ScreenColor(tui_color_yellow, tui_color_gray_scale[6]);
}

// ============================================================
// Window setup
// ============================================================

void MainTui::setup_windows() {
	int max_y, max_x;
	getmaxyx(stdscr, max_y, max_x);

	int title_h = 3;
	int status_h = 3;
	int content_h = max_y - title_h - status_h;
	int sidebar_w = 20;
	int content_w = max_x - sidebar_w;

	title_bar = newwin(title_h, max_x, 0, 0);
	sidebar   = newwin(content_h, sidebar_w, title_h, 0);
	content   = newwin(content_h, content_w, title_h, sidebar_w);
	status_bar = newwin(status_h, max_x, title_h + content_h, 0);

	attr_t bg_attr = bg_color.get();
	wbkgd(title_bar, bg_attr);
	wbkgd(sidebar, bg_attr);
	wbkgd(content, bg_attr);
	wbkgd(status_bar, bg_attr);

	keypad(sidebar, TRUE);
	keypad(content, TRUE);
	keypad(stdscr, TRUE);
}

void MainTui::destroy_windows() {
	if (title_bar) { delwin(title_bar); title_bar = nullptr; }
	if (sidebar)   { delwin(sidebar);   sidebar = nullptr; }
	if (content)   { delwin(content);   content = nullptr; }
	if (status_bar){ delwin(status_bar); status_bar = nullptr; }
}

// ============================================================
// Data helpers
// ============================================================

void MainTui::build_menu() {
	menu_items.clear();
	if (!logged_in) {
		menu_items.push_back({ "Login",   MainView::Login });
		menu_items.push_back({ "About",   MainView::About });
	} else {
		menu_items.push_back({ "User Info", MainView::UserInfo });
		menu_items.push_back({ "Bag",       MainView::Bag });
		menu_items.push_back({ "Shop",      MainView::Bag }); // action
		menu_items.push_back({ "Workers",   MainView::Game });
		menu_items.push_back({ "Password",  MainView::Password });
		menu_items.push_back({ "Logout",    MainView::Bag }); // action
		menu_items.push_back({ "Exit",      MainView::Bag }); // action
	}
	if (menu_selected >= (int)menu_items.size())
		menu_selected = 0;
}

MainView MainTui::view_for_menu(int idx) {
	if (idx < 0 || idx >= (int)menu_items.size())
		return MainView::UserInfo;
	return menu_items[idx].view;
}

void MainTui::build_bag_items() {
	bag_items.clear();
	for (auto& [name, node] : bag.childs) {
		if (name == "password") continue;
		if (!node.contains_value("count")) continue;
		int cnt = node.get_value<int>("count");
		if (cnt > 0)
			bag_items.push_back({ name, cnt });
	}
	std::sort(bag_items.begin(), bag_items.end(),
		[](const BagItem& a, const BagItem& b) { return a.name < b.name; });
	if (bag_selected >= (int)bag_items.size())
		bag_selected = 0;
}

void MainTui::load_item_defs() {
	item_arts.clear();
	item_descriptions.clear();
	std::filesystem::path items_dir("cache/items");
	if (!std::filesystem::exists(items_dir) || !std::filesystem::is_directory(items_dir))
		return;

	for (auto& entry : std::filesystem::recursive_directory_iterator(items_dir)) {
		if (entry.is_regular_file() && entry.path().extension() == ".wh") {
			CacheData file_items;
			file_items.cache_get(entry.path().string());
			for (auto& [name, def] : file_items.childs) {
				if (def.contains_value("art"))
					item_arts[name] = strip_quotes(def.get_value<std::string>("art"));
				if (def.contains_value("description"))
					item_descriptions[name] = strip_quotes(def.get_value<std::string>("description"));
			}
		}
	}
}

void MainTui::load_user_list() {
	login_user_list.clear();
	std::filesystem::path cache_dir("cache");
	if (!std::filesystem::exists(cache_dir) || !std::filesystem::is_directory(cache_dir))
		return;

	for (auto& entry : std::filesystem::directory_iterator(cache_dir)) {
		if (!entry.is_directory()) continue;
		std::string name = entry.path().filename().string();
		if (name == "items" || name == "shop") continue;
		std::string data_file = entry.path().string() + "/data.wh";
		if (std::filesystem::exists(data_file))
			login_user_list.push_back(name);
	}

	std::sort(login_user_list.begin(), login_user_list.end());
	if (login_user_selected >= (int)login_user_list.size())
		login_user_selected = 0;
}

// ============================================================
// Drawing
// ============================================================

void MainTui::draw_title() {
	wclear(title_bar);
	int max_x = getmaxx(title_bar);

	std::string title = "=== WHISKEY SHOP ===";
	int tx = (max_x - (int)title.size()) / 2;

	title_color.enable(title_bar);
	mvwprintw(title_bar, 1, tx, "%s", title.c_str());
	title_color.disable(title_bar);

	sidebar_color.enable(title_bar);
	box(title_bar, 0, 0);
	sidebar_color.disable(title_bar);

	wnoutrefresh(title_bar);
}

void MainTui::draw_sidebar() {
	wclear(sidebar);
	int max_y = getmaxy(sidebar);
	int max_x = getmaxx(sidebar);

	for (int i = 0; i < (int)menu_items.size() && i < max_y - 2; i++) {
		bool is_sel = (i == menu_selected && sidebar_active);
		ScreenColor& col = is_sel ? sidebar_selected_color : sidebar_normal_color;

		col.enable(sidebar);

		std::string prefix = is_sel ? " > " : "   ";
		std::string line = prefix + menu_items[i].label;

		int len = (int)line.size();
		if (len < max_x - 2)
			line += std::string(max_x - 2 - len, ' ');
		else
			line = line.substr(0, max_x - 2);

		mvwprintw(sidebar, i + 1, 0, "%s", line.c_str());
		col.disable(sidebar);
	}

	sidebar_color.enable(sidebar);
	box(sidebar, 0, 0);
	sidebar_color.disable(sidebar);

	wnoutrefresh(sidebar);
}

void MainTui::draw_content() {
	wclear(content);

	// CLI panel overrides right side
	if (cli_active) {
		draw_cli_panel();
		wnoutrefresh(content);
		return;
	}

	if (sidebar_active) {
		// Preview the view the sidebar is highlighting
		if (menu_selected >= 0 && menu_selected < (int)menu_items.size()) {
			std::string label = menu_items[menu_selected].label;
			if (label == "Shop") {
				draw_shop_preview();
			} else if (label == "Workers") {
				draw_game_preview();
			} else if (label == "Logout") {
				draw_action_preview("Logout", "Press Enter to log out.");
			} else if (label == "Exit") {
				draw_action_preview("Exit", "Press Enter to quit.");
			} else if (label == "Login") {
				draw_login_view();
			} else {
				// User Info, Bag, Password, About
				switch (menu_items[menu_selected].view) {
					case MainView::Bag:      draw_bag_view(); break;
					case MainView::Password: draw_password_view(); break;
					case MainView::About:    draw_about_view(); break;
					case MainView::UserInfo: draw_userinfo_view(); break;
					default: draw_userinfo_view(); break;
				}
			}
		}
	} else {
		// Active view (Enter was pressed)
		switch (current_view) {
			case MainView::Login:     draw_login_view();     break;
			case MainView::Bag:       draw_bag_view();       break;
			case MainView::Password:  draw_password_view();   break;
			case MainView::About:     draw_about_view();      break;
			case MainView::UserInfo:  draw_userinfo_view();   break;
			case MainView::Game:      draw_game_preview();    break;
			case MainView::MinerMenu: draw_miner_menu();      break;
		}
	}

	content_color.enable(content);
	box(content, 0, 0);
	content_color.disable(content);

	wnoutrefresh(content);
}

void MainTui::draw_status() {
	wclear(status_bar);
	int max_x = getmaxx(status_bar);

	status_color.enable(status_bar);

	std::string help_text;
	if (sidebar_active)
		help_text = "[UP/DOWN] Navigate  [Enter] Select  [?] Help  [q] Exit";
	else if (cli_active && cli_editing)
		help_text = "[ESC] Deactivate  [Enter] Execute  [!] Close terminal";
	else if (cli_active)
		help_text = "[!] Close terminal  [Enter] Activate  [q] Exit";
	else if (login_editing)
		help_text = "[Enter] Login  [Esc] Back  [?] Help";
	else if (login_creating)
		help_text = "[Tab] Field  [Enter] Create  [Esc] Cancel  [?] Help";
	else if (current_view == MainView::Login)
		help_text = "[UP/DOWN] Select  [Enter] Login  [d] Delete  [?] Help";
	else
		help_text = "[ESC/RIGHT] Back  [!] Terminal  [?] Help  [q] Exit";

	int hx = (max_x - (int)help_text.size()) / 2;
	mvwprintw(status_bar, 0, hx > 0 ? hx : 0, "%s", help_text.c_str());

	if (logged_in) {
		std::string user_info = "User: " + username + "  Coins: " + std::to_string(coins);
		int ux = (max_x - (int)user_info.size()) / 2;
		mvwprintw(status_bar, 1, ux > 0 ? ux : 0, "%s", user_info.c_str());
	} else {
		std::string info = "Not logged in";
		int ix = (max_x - (int)info.size()) / 2;
		mvwprintw(status_bar, 1, ix > 0 ? ix : 0, "%s", info.c_str());
	}

	status_color.disable(status_bar);

	sidebar_color.enable(status_bar);
	box(status_bar, 0, 0);
	sidebar_color.disable(status_bar);

	wnoutrefresh(status_bar);
}

void MainTui::draw_all() {
	draw_title();
	draw_sidebar();
	draw_content();
	draw_status();
	if (show_help)
		draw_help_overlay();
	doupdate();
}

// ============================================================
// Content views
// ============================================================

void MainTui::draw_login_view() {
	int max_y = getmaxy(content);
	int max_x = getmaxx(content);

	highlight_color.enable(content);
	std::string title = "-- Select User --";
	int tx = (max_x - (int)title.size()) / 2;
	mvwprintw(content, 1, tx, "%s", title.c_str());
	highlight_color.disable(content);

	int start_y = 3;
	int field_w = 30;
	int field_x = (max_x - field_w) / 2;

	if (login_creating) {
		highlight_color.enable(content);
		std::string cr_title = "-- Create New User --";
		int crx = (max_x - (int)cr_title.size()) / 2;
		mvwprintw(content, start_y, crx, "%s", cr_title.c_str());
		highlight_color.disable(content);

		start_y += 2;
		label_color.enable(content);
		mvwprintw(content, start_y, field_x, "Username:");
		label_color.disable(content);

		start_y++;
		bool user_active = (login_field == 0);
		ScreenColor& ucol = user_active ? input_active_color : input_color;
		ucol.enable(content);
		std::string user_display = login_new_user;
		if (user_active) user_display += "_";
		int ulen = (int)user_display.size();
		if (ulen < field_w) user_display += std::string(field_w - ulen, ' ');
		else user_display = user_display.substr(0, field_w);
		mvwprintw(content, start_y, field_x, "%s", user_display.c_str());
		ucol.disable(content);

		start_y += 2;
		label_color.enable(content);
		mvwprintw(content, start_y, field_x, "Password (optional):");
		label_color.disable(content);

		start_y++;
		bool pass_active = (login_field == 1);
		ScreenColor& pcol = pass_active ? input_active_color : input_color;
		pcol.enable(content);
		std::string pass_display(login_pass.size(), '*');
		if (pass_active) pass_display += "_";
		int plen = (int)pass_display.size();
		if (plen < field_w) pass_display += std::string(field_w - plen, ' ');
		else pass_display = pass_display.substr(0, field_w);
		mvwprintw(content, start_y, field_x, "%s", pass_display.c_str());
		pcol.disable(content);

		start_y += 2;
		status_color.enable(content);
		std::string help = "[Tab] Field  [Enter] Create  [Esc] Cancel";
		int hx = (max_x - (int)help.size()) / 2;
		mvwprintw(content, start_y, hx > 0 ? hx : 0, "%s", help.c_str());
		status_color.disable(content);
		return;
	}

	if (login_editing) {
		highlight_color.enable(content);
		std::string pass_title = "-- Password for " + login_user + " --";
		int ptx = (max_x - (int)pass_title.size()) / 2;
		mvwprintw(content, start_y, ptx, "%s", pass_title.c_str());
		highlight_color.disable(content);

		start_y += 3;
		label_color.enable(content);
		mvwprintw(content, start_y, field_x, "Password:");
		label_color.disable(content);

		start_y++;
		ScreenColor& pcol = input_active_color;
		pcol.enable(content);
		std::string pass_display(login_pass.size(), '*');
		pass_display += "_";
		int plen = (int)pass_display.size();
		if (plen < field_w) pass_display += std::string(field_w - plen, ' ');
		else pass_display = pass_display.substr(0, field_w);
		mvwprintw(content, start_y, field_x, "%s", pass_display.c_str());
		pcol.disable(content);

		start_y += 3;
		status_color.enable(content);
		std::string help = "[Enter] Login  [Esc] Back";
		int hx = (max_x - (int)help.size()) / 2;
		mvwprintw(content, start_y, hx > 0 ? hx : 0, "%s", help.c_str());
		status_color.disable(content);
		return;
	}

	// User list mode
	int list_x = 3;
	int list_w = max_x - 6;
	int list_top = start_y;
	int list_bot = max_y - 4;
	int visible = list_bot - list_top;
	if (visible < 1) visible = 1;

	// Total items: + Add User, users..., x Delete User
	int total_items = 1 + (int)login_user_list.size();
	bool has_delete = !login_user_list.empty();
	if (has_delete) total_items++;

	if (login_user_selected < 0) login_user_selected = 0;
	if (login_user_selected >= total_items) login_user_selected = total_items - 1;

	int scroll = 0;
	if (login_user_selected >= visible)
		scroll = login_user_selected - visible + 1;

	for (int i = 0; i < visible && (scroll + i) < total_items; i++) {
		int idx = scroll + i;
		bool is_sel = (idx == login_user_selected);
		ScreenColor& col = is_sel ? sidebar_selected_color : item_color;
		col.enable(content);

		std::string line;
		if (idx == 0) {
			line = "+ Add User";
		} else if (idx <= (int)login_user_list.size()) {
			line = "  " + login_user_list[idx - 1];
		} else {
			line = "x Delete User";
		}

		if (is_sel) {
			line[0] = '>';
		}

		if ((int)line.size() < list_w) {
			line += std::string(list_w - line.size(), ' ');
		} else {
			line = line.substr(0, list_w);
		}

		mvwprintw(content, list_top + i, list_x, "%s", line.c_str());
		col.disable(content);
	}

	if (total_items == 0) {
		status_color.enable(content);
		mvwprintw(content, list_top, list_x, "(no users found)");
		status_color.disable(content);
	}

	start_y = list_bot + 1;
	status_color.enable(content);
	std::string help = "[UP/DOWN] Select  [Enter] Login  [?] Help  [q] Exit";
	int hx = (max_x - (int)help.size()) / 2;
	mvwprintw(content, start_y, hx > 0 ? hx : 0, "%s", help.c_str());
	status_color.disable(content);
}

void MainTui::draw_bag_view() {
	int max_y = getmaxy(content);
	int max_x = getmaxx(content);

	int split_x = max_x / 2;

	highlight_color.enable(content);
	std::string title = "--- Bag (" + username + ") ---";
	int tx = (split_x - (int)title.size()) / 2;
	if (tx < 2) tx = 2;
	mvwprintw(content, 1, tx, "%s", title.c_str());
	highlight_color.disable(content);

	coin_color.enable(content);
	mvwprintw(content, 2, 3, "Coins: %d", coins);
	coin_color.disable(content);

	status_color.enable(content);
	for (int x = 1; x < split_x - 1; x++)
		mvwaddch(content, 3, x, ACS_HLINE);
	status_color.disable(content);

	int visible = max_y - 5;
	if (visible < 1) visible = 1;
	if (bag_selected < 0) bag_selected = 0;
	if (bag_selected >= (int)bag_items.size() && !bag_items.empty())
		bag_selected = (int)bag_items.size() - 1;

	int scroll = 0;
	if (bag_selected >= visible)
		scroll = bag_selected - visible + 1;

	for (int i = 0; i < visible && (scroll + i) < (int)bag_items.size(); i++) {
		int idx = scroll + i;
		auto& bi = bag_items[idx];
		bool is_sel = (idx == bag_selected && !sidebar_active);

		ScreenColor& col = is_sel ? sidebar_selected_color : item_color;
		col.enable(content);

		char line[256];
		snprintf(line, sizeof(line), "%c %-13s x%-4d",
			is_sel ? '>' : ' ', bi.name.c_str(), bi.count);

		int len = strlen(line);
		if (len < split_x - 2) {
			memset(line + len, ' ', split_x - 2 - len);
			line[split_x - 2] = '\0';
		} else {
			line[split_x - 2] = '\0';
		}

		mvwprintw(content, 4 + i, 0, "%s", line);
		col.disable(content);
	}

	if (bag_items.empty()) {
		status_color.enable(content);
		mvwprintw(content, 4, 3, "(empty)");
		status_color.disable(content);
	}

	// Vertical separator
	status_color.enable(content);
	for (int y = 1; y < max_y - 1; y++)
		mvwaddch(content, y, split_x, ACS_VLINE);
	status_color.disable(content);

	sidebar_color.enable(content);
	mvwaddch(content, 0, split_x, ACS_TTEE);
	mvwaddch(content, max_y - 1, split_x, ACS_BTEE);
	sidebar_color.disable(content);

	if (!bag_items.empty() && bag_selected >= 0 && bag_selected < (int)bag_items.size()) {
		auto& bi = bag_items[bag_selected];

		highlight_color.enable(content);
		std::string art_title = "--- " + bi.name + " ---";
		int atx = split_x + 1 + (max_x - split_x - 1 - (int)art_title.size()) / 2;
		if (atx <= split_x + 1) atx = split_x + 2;
		mvwprintw(content, 1, atx, "%s", art_title.c_str());
		highlight_color.disable(content);

		int art_end = 3;
		auto it = item_arts.find(bi.name);
		if (it != item_arts.end()) {
			art_end = load_art(it->second, content, 3, max_y / 2 - 2, split_x + 3);
		} else {
			art_color.enable(content);
			mvwprintw(content, max_y / 2, split_x + 3, "[no art]");
			art_color.disable(content);
			art_end = max_y / 2 + 1;
		}

		auto di = item_descriptions.find(bi.name);
		if (di != item_descriptions.end() && !di->second.empty()) {
			std::ifstream desc_file(di->second);
			if (desc_file.is_open()) {
				status_color.enable(content);
				int y = art_end + 2;
				std::string dline;
				while (std::getline(desc_file, dline) && y < max_y - 2) {
					int panel_w = max_x - split_x - 3;
					if ((int)dline.size() > panel_w)
						dline = dline.substr(0, panel_w);
					mvwprintw(content, y, split_x + 2, "%s", dline.c_str());
					y++;
				}
				status_color.disable(content);
			}
		}
	} else {
		art_color.enable(content);
		mvwprintw(content, max_y / 2, split_x + 3, "[ select an item ]");
		art_color.disable(content);
	}
}

void MainTui::draw_password_view() {
	int max_x = getmaxx(content);

	int start_y = 3;
	int field_w = 30;
	int field_x = (max_x - field_w) / 2;

	highlight_color.enable(content);
	std::string title = "-- Change Password --";
	int tx = (max_x - (int)title.size()) / 2;
	mvwprintw(content, start_y, tx, "%s", title.c_str());
	highlight_color.disable(content);

	start_y += 3;
	label_color.enable(content);
	mvwprintw(content, start_y, field_x, "New Password:");
	label_color.disable(content);

	start_y++;
	ScreenColor& pcol = pass_editing ? input_active_color : input_color;
	pcol.enable(content);
	std::string pass_display(new_pass.size(), '*');
	if (pass_editing) pass_display += "_";
	int plen = (int)pass_display.size();
	if (plen < field_w) pass_display += std::string(field_w - plen, ' ');
	else pass_display = pass_display.substr(0, field_w);
	mvwprintw(content, start_y, field_x, "%s", pass_display.c_str());
	pcol.disable(content);

	start_y += 3;
	status_color.enable(content);
	std::string help = "[Enter] Save  [Esc] Cancel";
	int hx = (max_x - (int)help.size()) / 2;
	mvwprintw(content, start_y, hx > 0 ? hx : 0, "%s", help.c_str());
	status_color.disable(content);
}

void MainTui::draw_about_view() {
	int max_x = getmaxx(content);

	int start_y = 3;

	highlight_color.enable(content);
	std::string title = "-- About --";
	int tx = (max_x - (int)title.size()) / 2;
	mvwprintw(content, start_y, tx, "%s", title.c_str());
	highlight_color.disable(content);

	start_y += 3;
	item_color.enable(content);
	mvwprintw(content, start_y, 3, "Whiskey Shop");
	start_y += 2;
	mvwprintw(content, start_y, 3, "A virtual shop game.");
	start_y += 2;
	mvwprintw(content, start_y, 3, "Buy and sell items");
	start_y++;
	mvwprintw(content, start_y, 3, "at daily changing prices.");
	start_y += 3;
	mvwprintw(content, start_y, 3, "Items:");
	start_y++;
	mvwprintw(content, start_y, 3, "  sword, shield, potion,");
	start_y++;
	mvwprintw(content, start_y, 3, "  armor, boots, bow,");
	start_y++;
	mvwprintw(content, start_y, 3, "  gloves, helmet, ring,");
	start_y++;
	mvwprintw(content, start_y, 3, "  amulet, whiskey,");
	start_y++;
	mvwprintw(content, start_y, 3, "  dragon-head, garden,");
	start_y++;
	mvwprintw(content, start_y, 3, "  gold");
	item_color.disable(content);
}

void MainTui::draw_userinfo_view() {
	int max_x = getmaxx(content);

	int start_y = 3;

	highlight_color.enable(content);
	std::string title = "-- User Info --";
	int tx = (max_x - (int)title.size()) / 2;
	mvwprintw(content, start_y, tx, "%s", title.c_str());
	highlight_color.disable(content);

	start_y += 3;
	label_color.enable(content);
	mvwprintw(content, start_y, 5, "Username:");
	label_color.disable(content);
	item_color.enable(content);
	mvwprintw(content, start_y, 20, "%s", username.c_str());
	item_color.disable(content);

	start_y += 2;
	coin_color.enable(content);
	mvwprintw(content, start_y, 5, "Coins: %d", coins);
	coin_color.disable(content);

	start_y += 2;
	int item_count = 0;
	for (auto& bi : bag_items)
		item_count += bi.count;
	label_color.enable(content);
	mvwprintw(content, start_y, 5, "Total items:");
	label_color.disable(content);
	item_color.enable(content);
	mvwprintw(content, start_y, 20, "%d", item_count);
	item_color.disable(content);
}

void MainTui::draw_shop_preview() {
	int max_x = getmaxx(content);

	int start_y = 3;

	highlight_color.enable(content);
	std::string title = "-- Shop --";
	int tx = (max_x - (int)title.size()) / 2;
	mvwprintw(content, start_y, tx, "%s", title.c_str());
	highlight_color.disable(content);

	start_y += 3;
	item_color.enable(content);
	mvwprintw(content, start_y, 3, "Enter the shop to buy");
	start_y++;
	mvwprintw(content, start_y, 3, "and sell items.");
	start_y += 2;
	mvwprintw(content, start_y, 3, "Prices change daily!");
	start_y += 2;
	mvwprintw(content, start_y, 3, "Press Enter to open shop.");
	item_color.disable(content);
}

void MainTui::draw_game_preview() {
	int max_x = getmaxx(content);
	int max_y = getmaxy(content);

	int start_y = 2;

	highlight_color.enable(content);
	std::string title = "-- Miner --";
	int tx = (max_x - (int)title.size()) / 2;
	mvwprintw(content, start_y, tx, "%s", title.c_str());
	highlight_color.disable(content);

	start_y += 2;

	if (!logged_in) {
		item_color.enable(content);
		mvwprintw(content, start_y, 3, "Login first to play!");
		item_color.disable(content);
		return;
	}

	// Read miner data from bag
	int m_level = 1;
	int m_gpm = 2;
	int m_max_workers = 5;
	int m_workers = 0;
	int m_max_gold = 100;
	time_t m_last = time(nullptr);
	int gold_in_bag = 0;

	if (bag.contains_child("miner")) {
		CacheData& m = bag.get_child("miner");
		if (m.contains_value("level"))         m_level = m.get_value<int>("level");
		if (m.contains_value("gold_per_minute")) m_gpm = m.get_value<int>("gold_per_minute");
		if (m.contains_value("max_workers"))   m_max_workers = m.get_value<int>("max_workers");
		if (m.contains_value("workers"))       m_workers = m.get_value<int>("workers");
		if (m.contains_value("max_gold"))      m_max_gold = m.get_value<int>("max_gold");
		if (m.contains_value("last_collect_time")) m_last = (time_t)m.get_value<long long>("last_collect_time");
	}
	if (bag.contains_child("gold") && bag.get_child("gold").contains_value("count"))
		gold_in_bag = bag.get_child("gold").get_value<int>("count");

	// Calculate current gold
	double current_gold = 0.0;
	if (m_workers > 0 && m_gpm > 0) {
		double elapsed = difftime(time(nullptr), m_last);
		current_gold = (elapsed / 60.0) * m_gpm * m_workers;
		if (current_gold < 0) current_gold = 0;
		if (current_gold > m_max_gold) current_gold = (double)m_max_gold;
	}
	int total_income = m_gpm * m_workers;

	// Draw stats
	label_color.enable(content);
	mvwprintw(content, start_y, 3, "Level:");
	label_color.disable(content);
	coin_color.enable(content);
	mvwprintw(content, start_y, 20, "%d / 100", m_level);
	coin_color.disable(content);
	start_y++;

	label_color.enable(content);
	mvwprintw(content, start_y, 3, "Gold/min (each):");
	label_color.disable(content);
	highlight_color.enable(content);
	mvwprintw(content, start_y, 20, "%d", m_gpm);
	highlight_color.disable(content);
	start_y++;

	label_color.enable(content);
	mvwprintw(content, start_y, 3, "Total income:");
	label_color.disable(content);
	highlight_color.enable(content);
	mvwprintw(content, start_y, 20, "%d / min", total_income);
	highlight_color.disable(content);
	start_y += 2;

	label_color.enable(content);
	mvwprintw(content, start_y, 3, "Workers:");
	label_color.disable(content);
	coin_color.enable(content);
	mvwprintw(content, start_y, 20, "%d / %d", m_workers, m_max_workers);
	coin_color.disable(content);
	start_y++;

	label_color.enable(content);
	mvwprintw(content, start_y, 3, "Max gold cap:");
	label_color.disable(content);
	highlight_color.enable(content);
	mvwprintw(content, start_y, 20, "%d", m_max_gold);
	highlight_color.disable(content);
	start_y += 2;

	label_color.enable(content);
	mvwprintw(content, start_y, 3, "Available gold:");
	label_color.disable(content);
	highlight_color.enable(content);
	mvwprintw(content, start_y, 20, "%d", (int)current_gold);
	highlight_color.disable(content);
	start_y++;

	label_color.enable(content);
	mvwprintw(content, start_y, 3, "Gold in bag:");
	label_color.disable(content);
	highlight_color.enable(content);
	mvwprintw(content, start_y, 20, "%d", gold_in_bag);
	highlight_color.disable(content);
	start_y++;

	label_color.enable(content);
	mvwprintw(content, start_y, 3, "Coins:");
	label_color.disable(content);
	coin_color.enable(content);
	mvwprintw(content, start_y, 20, "%d", coins);
	coin_color.disable(content);
	start_y += 2;

	item_color.enable(content);
	mvwprintw(content, start_y, 3, "Press Enter to manage miners.");
	item_color.disable(content);
}

// ============================================================
// Miner Menu
// ============================================================

void MainTui::load_miner_data() {
	if (username.empty()) return;
	std::string path = "cache/" + username + "/data.wh";
	bag.cache_get(path);
	if (!bag.contains_value("coins"))
		bag.set_value("coins", "100");
	coins = bag.get_value<int>("coins");
	build_bag_items();
}

void MainTui::miner_collect() {
	load_miner_data();

	int m_workers = 0, m_gpm = 2, m_max_gold = 100;
	time_t m_last = time(nullptr);

	if (bag.contains_child("miner")) {
		CacheData& m = bag.get_child("miner");
		if (m.contains_value("workers"))       m_workers = m.get_value<int>("workers");
		if (m.contains_value("gold_per_minute")) m_gpm = m.get_value<int>("gold_per_minute");
		if (m.contains_value("max_gold"))      m_max_gold = m.get_value<int>("max_gold");
		if (m.contains_value("last_collect_time")) m_last = (time_t)m.get_value<long long>("last_collect_time");
	}

	if (m_workers <= 0) {
		miner_status_msg = "No workers mining!";
		return;
	}

	double elapsed = difftime(time(nullptr), m_last);
	double gold = (elapsed / 60.0) * m_gpm * m_workers;
	if (gold < 0) gold = 0;
	if (gold > m_max_gold) gold = (double)m_max_gold;

	int collect = (int)gold;
	if (collect <= 0) {
		miner_status_msg = "Nothing to collect yet.";
		return;
	}

	int gold_in_bag = 0;
	if (bag.contains_child("gold") && bag.get_child("gold").contains_value("count"))
		gold_in_bag = bag.get_child("gold").get_value<int>("count");

	bag.childs["gold"].set_value("count", gold_in_bag + collect);
	bag.childs["miner"].set_value("last_collect_time", (long long)time(nullptr));

	std::string dir = "cache/" + username;
	std::filesystem::create_directories(dir);
	bag.cache_put(dir + "/data.wh");

	coins = bag.get_value<int>("coins");
	build_bag_items();
	miner_status_msg = "Collected " + std::to_string(collect) + " gold!";
}

void MainTui::miner_buy_worker() {
	load_miner_data();

	int m_workers = 0, m_max_workers = 5;
	int cost = 50;

	if (bag.contains_child("miner")) {
		CacheData& m = bag.get_child("miner");
		if (m.contains_value("workers"))     m_workers = m.get_value<int>("workers");
		if (m.contains_value("max_workers")) m_max_workers = m.get_value<int>("max_workers");
	}

	cost = 50 * (m_workers + 1);

	if (m_workers >= m_max_workers) {
		miner_status_msg = "Max workers reached!";
		return;
	}
	if (cost > coins) {
		miner_status_msg = "Not enough coins! Need " + std::to_string(cost);
		return;
	}

	coins -= cost;
	bag.set_value("coins", coins);
	bag.childs["miner"].set_value("workers", m_workers + 1);
	if (m_workers == 0)
		bag.childs["miner"].set_value("last_collect_time", (long long)time(nullptr));

	std::string dir = "cache/" + username;
	std::filesystem::create_directories(dir);
	bag.cache_put(dir + "/data.wh");

	build_bag_items();
	miner_status_msg = "Hired worker! (-" + std::to_string(cost) + " coins)";
}

void MainTui::miner_sell_worker() {
	load_miner_data();

	int m_workers = 0;
	if (bag.contains_child("miner")) {
		CacheData& m = bag.get_child("miner");
		if (m.contains_value("workers")) m_workers = m.get_value<int>("workers");
	}

	if (m_workers <= 0) {
		miner_status_msg = "No workers to fire!";
		return;
	}

	int refund = 50 * m_workers / 2;
	coins += refund;
	bag.set_value("coins", coins);
	bag.childs["miner"].set_value("workers", m_workers - 1);
	if (m_workers == 1)
		bag.childs["miner"].set_value("last_collect_time", (long long)time(nullptr));

	std::string dir = "cache/" + username;
	std::filesystem::create_directories(dir);
	bag.cache_put(dir + "/data.wh");

	build_bag_items();
	miner_status_msg = "Fired worker! (+" + std::to_string(refund) + " coins)";
}

void MainTui::miner_level_up() {
	load_miner_data();

	int m_level = 1, m_gpm = 2, m_max_workers = 5, m_max_gold = 100;
	int gold_in_bag = 0;

	if (bag.contains_child("miner")) {
		CacheData& m = bag.get_child("miner");
		if (m.contains_value("level"))           m_level = m.get_value<int>("level");
		if (m.contains_value("gold_per_minute")) m_gpm = m.get_value<int>("gold_per_minute");
		if (m.contains_value("max_workers"))     m_max_workers = m.get_value<int>("max_workers");
		if (m.contains_value("max_gold"))        m_max_gold = m.get_value<int>("max_gold");
	}
	if (bag.contains_child("gold") && bag.get_child("gold").contains_value("count"))
		gold_in_bag = bag.get_child("gold").get_value<int>("count");

	if (m_level >= 100) {
		miner_status_msg = "Max level reached!";
		return;
	}

	int cost = m_level * 5;
	if (cost > gold_in_bag) {
		miner_status_msg = "Need " + std::to_string(cost) + " gold to level up!";
		return;
	}

	bag.childs["gold"].set_value("count", gold_in_bag - cost);
	if (bag.get_child("gold").get_value<int>("count") <= 0)
		bag.childs.erase("gold");

	int new_gpm = (int)(m_gpm * 1.5);
	if (new_gpm < 1) new_gpm = 1;

	bag.childs["miner"].set_value("level", m_level + 1);
	bag.childs["miner"].set_value("gold_per_minute", new_gpm);
	bag.childs["miner"].set_value("max_workers", m_max_workers + 5);
	bag.childs["miner"].set_value("max_gold", m_max_gold + 50);

	std::string dir = "cache/" + username;
	std::filesystem::create_directories(dir);
	bag.cache_put(dir + "/data.wh");

	coins = bag.get_value<int>("coins");
	build_bag_items();
	miner_status_msg = "Level up! Now level " + std::to_string(m_level + 1) + "!";
}

void MainTui::draw_miner_menu() {
	int max_x = getmaxx(content);
	int max_y = getmaxy(content);

	int start_y = 1;

	highlight_color.enable(content);
	std::string title = "-- Miner --";
	int tx = (max_x - (int)title.size()) / 2;
	mvwprintw(content, start_y, tx, "%s", title.c_str());
	highlight_color.disable(content);
	start_y += 2;

	// Read miner stats
	int m_level = 1, m_gpm = 2, m_max_workers = 5, m_workers = 0, m_max_gold = 100;
	time_t m_last = time(nullptr);
	int gold_in_bag = 0;

	if (bag.contains_child("miner")) {
		CacheData& m = bag.get_child("miner");
		if (m.contains_value("level"))           m_level = m.get_value<int>("level");
		if (m.contains_value("gold_per_minute")) m_gpm = m.get_value<int>("gold_per_minute");
		if (m.contains_value("max_workers"))     m_max_workers = m.get_value<int>("max_workers");
		if (m.contains_value("workers"))         m_workers = m.get_value<int>("workers");
		if (m.contains_value("max_gold"))        m_max_gold = m.get_value<int>("max_gold");
		if (m.contains_value("last_collect_time")) m_last = (time_t)m.get_value<long long>("last_collect_time");
	}
	if (bag.contains_child("gold") && bag.get_child("gold").contains_value("count"))
		gold_in_bag = bag.get_child("gold").get_value<int>("count");

	double current_gold = 0.0;
	if (m_workers > 0 && m_gpm > 0) {
		double elapsed = difftime(time(nullptr), m_last);
		current_gold = (elapsed / 60.0) * m_gpm * m_workers;
		if (current_gold < 0) current_gold = 0;
		if (current_gold > m_max_gold) current_gold = (double)m_max_gold;
	}

	// Stats panel (left half)
	int split_x = max_x * 2 / 3;

	status_color.enable(content);
	for (int y = 1; y < max_y - 1; y++)
		mvwaddch(content, y, split_x, ACS_VLINE);
	status_color.disable(content);

	// Left: stats
	label_color.enable(content);
	mvwprintw(content, start_y, 3, "Level:");
	label_color.disable(content);
	coin_color.enable(content);
	mvwprintw(content, start_y, 18, "%d / 100", m_level);
	coin_color.disable(content);
	start_y++;

	label_color.enable(content);
	mvwprintw(content, start_y, 3, "Gold/min (each):");
	label_color.disable(content);
	highlight_color.enable(content);
	mvwprintw(content, start_y, 18, "%d", m_gpm);
	highlight_color.disable(content);
	start_y++;

	label_color.enable(content);
	mvwprintw(content, start_y, 3, "Total income:");
	label_color.disable(content);
	highlight_color.enable(content);
	mvwprintw(content, start_y, 18, "%d / min", m_gpm * m_workers);
	highlight_color.disable(content);
	start_y++;

	label_color.enable(content);
	mvwprintw(content, start_y, 3, "Workers:");
	label_color.disable(content);
	coin_color.enable(content);
	mvwprintw(content, start_y, 18, "%d / %d", m_workers, m_max_workers);
	coin_color.disable(content);
	start_y++;

	label_color.enable(content);
	mvwprintw(content, start_y, 3, "Max gold cap:");
	label_color.disable(content);
	highlight_color.enable(content);
	mvwprintw(content, start_y, 18, "%d", m_max_gold);
	highlight_color.disable(content);
	start_y += 2;

	highlight_color.enable(content);
	mvwprintw(content, start_y, 3, "Available gold:");
	highlight_color.disable(content);
	highlight_color.enable(content);
	mvwprintw(content, start_y, 18, "%d", (int)current_gold);
	highlight_color.disable(content);
	start_y++;

	highlight_color.enable(content);
	mvwprintw(content, start_y, 3, "Gold in bag:");
	highlight_color.disable(content);
	highlight_color.enable(content);
	mvwprintw(content, start_y, 18, "%d", gold_in_bag);
	highlight_color.disable(content);
	start_y++;

	coin_color.enable(content);
	mvwprintw(content, start_y, 3, "Coins:");
	coin_color.disable(content);
	coin_color.enable(content);
	mvwprintw(content, start_y, 18, "%d", coins);
	coin_color.disable(content);
	start_y += 2;

	// Status message
	if (!miner_status_msg.empty()) {
		highlight_color.enable(content);
		mvwprintw(content, start_y, 3, "%s", miner_status_msg.c_str());
		highlight_color.disable(content);
		start_y++;
	}

	// Right: menu options
	int rx = split_x + 2;
	int ry = 2;

	highlight_color.enable(content);
	std::string menu_title = "--- Actions ---";
	mvwprintw(content, ry, rx, "%s", menu_title.c_str());
	highlight_color.disable(content);
	ry += 2;

	struct MinerOption {
		const char* label;
		const char* key_hint;
	};
	MinerOption options[] = {
		{ "Collect Gold",   "c" },
		{ "Buy Worker",     "w" },
		{ "Sell Worker",    "W" },
		{ "Level Up",       "l" },
		{ "Back",           "ESC" },
	};
	int num_options = 5;

	for (int i = 0; i < num_options; i++) {
		bool is_sel = (i == miner_menu_selected);
		ScreenColor& col = is_sel ? sidebar_selected_color : item_color;

		col.enable(content);
		char line[256];
		snprintf(line, sizeof(line), "%c %s", is_sel ? '>' : ' ', options[i].label);
		mvwprintw(content, ry, rx, "%-*s", 20, line);
		col.disable(content);

		status_color.enable(content);
		mvwprintw(content, ry, rx + 22, "[%s]", options[i].key_hint);
		status_color.disable(content);

		ry++;
	}

	// Cost info below options
	ry++;
	int worker_cost_val = 50 * (m_workers + 1);
	int level_cost_val = (m_level) * 5;
	item_color.enable(content);
	mvwprintw(content, ry, rx, "Worker cost: %d coins", worker_cost_val);
	ry++;
	mvwprintw(content, ry, rx, "Level cost:  %d gold", level_cost_val);
	item_color.disable(content);

	content_color.enable(content);
	box(content, 0, 0);
	content_color.disable(content);

	wnoutrefresh(content);
}

void MainTui::handle_miner_menu_keys(int key) {
	int num_options = 5;

	switch (key) {
		case KEY_UP:
		case 'k':
			miner_menu_selected = (miner_menu_selected + num_options - 1) % num_options;
			miner_status_msg.clear();
			break;
		case KEY_DOWN:
		case 'j':
			miner_menu_selected = (miner_menu_selected + 1) % num_options;
			miner_status_msg.clear();
			break;
		case '\n':
		case KEY_ENTER:
			switch (miner_menu_selected) {
				case 0: miner_collect(); break;
				case 1: miner_buy_worker(); break;
				case 2: miner_sell_worker(); break;
				case 3: miner_level_up(); break;
				case 4: // Back
					sidebar_active = true;
					current_view = MainView::Game;
					miner_status_msg.clear();
					break;
			}
			break;
		case 'c': miner_collect(); break;
		case 'w': miner_buy_worker(); break;
		case 'W': miner_sell_worker(); break;
		case 'l': miner_level_up(); break;
		case 27: // ESC
		case KEY_RIGHT:
			sidebar_active = true;
			current_view = MainView::Game;
			miner_status_msg.clear();
			break;
	}
}

void MainTui::draw_action_preview(const std::string& title, const std::string& desc) {
	int max_x = getmaxx(content);

	highlight_color.enable(content);
	std::string t = "-- " + title + " --";
	int tx = (max_x - (int)t.size()) / 2;
	mvwprintw(content, 3, tx, "%s", t.c_str());
	highlight_color.disable(content);

	item_color.enable(content);
	int dx = (max_x - (int)desc.size()) / 2;
	mvwprintw(content, 6, dx > 0 ? dx : 3, "%s", desc.c_str());
	item_color.disable(content);
}

// ============================================================
// CLI Panel
// ============================================================

void MainTui::draw_cli_panel() {
	int max_y = getmaxy(content);
	int max_x = getmaxx(content);
	int split_x = max_x / 2;
	int panel_w = max_x - split_x - 2;

	// Title
	highlight_color.enable(content);
	std::string title = "--- Terminal ---";
	int tx = split_x + 1 + (panel_w - (int)title.size()) / 2;
	mvwprintw(content, 1, tx, "%s", title.c_str());
	highlight_color.disable(content);

	// Separator under title
	status_color.enable(content);
	for (int x = split_x + 1; x < max_x - 1; x++)
		mvwaddch(content, 2, x, ACS_HLINE);
	status_color.disable(content);

	// History area: rows 3 to max_y-5
	int hist_top = 3;
	int hist_bot = max_y - 5;
	int hist_h = hist_bot - hist_top + 1;

	// Collect all history lines for scrolling
	struct HistLine {
		bool is_cmd;
		std::string text;
	};
	std::vector<HistLine> all_lines;
	for (auto& [cmd, out] : cli_history) {
		all_lines.push_back({true, "> " + cmd});
		if (!out.empty()) {
			std::istringstream iss(out);
			std::string ol;
			while (std::getline(iss, ol))
				all_lines.push_back({false, ol});
		}
		all_lines.push_back({false, ""});
	}

	// Scroll to bottom
	int total = (int)all_lines.size();
	if (total <= hist_h)
		cli_scroll = 0;
	else
		cli_scroll = total - hist_h;

	// Draw visible history
	for (int i = 0; i < hist_h && (cli_scroll + i) < total; i++) {
		auto& hl = all_lines[cli_scroll + i];
		std::string display = hl.text;
		if ((int)display.size() > panel_w)
			display = display.substr(0, panel_w);
		if (hl.is_cmd) {
			highlight_color.enable(content);
			mvwprintw(content, hist_top + i, split_x + 1, "%-*s", panel_w, display.c_str());
			highlight_color.disable(content);
		} else {
			item_color.enable(content);
			mvwprintw(content, hist_top + i, split_x + 1, "%-*s", panel_w, display.c_str());
			item_color.disable(content);
		}
	}

	// Empty state
	if (all_lines.empty()) {
		status_color.enable(content);
		std::string empty_msg = "(type 'help' for commands)";
		int emx = split_x + 1 + (panel_w - (int)empty_msg.size()) / 2;
		mvwprintw(content, hist_top + hist_h / 2, emx > split_x + 1 ? emx : split_x + 2, "%s", empty_msg.c_str());
		status_color.disable(content);
	}

	// Separator above input
	status_color.enable(content);
	for (int x = split_x + 1; x < max_x - 1; x++)
		mvwaddch(content, hist_bot + 1, x, ACS_HLINE);
	status_color.disable(content);

	// Input line
	int input_y = hist_bot + 2;
	std::string prompt = "whiskey> ";
	std::string display_input = cli_input;
	if (cli_editing) display_input += "_";
	std::string input_line = prompt + display_input;

	if ((int)input_line.size() > panel_w)
		input_line = input_line.substr((int)input_line.size() - panel_w);

	ScreenColor& icol = cli_editing ? input_active_color : input_color;
	icol.enable(content);
	mvwprintw(content, input_y, split_x + 1, "%-*s", panel_w, input_line.c_str());
	icol.disable(content);

	// Vertical separator
	status_color.enable(content);
	for (int y = 1; y < max_y - 1; y++)
		mvwaddch(content, y, split_x, ACS_VLINE);
	status_color.disable(content);

	sidebar_color.enable(content);
	mvwaddch(content, 0, split_x, ACS_TTEE);
	mvwaddch(content, max_y - 1, split_x, ACS_BTEE);
	sidebar_color.disable(content);
}

void MainTui::draw_help_overlay() {
	int max_y, max_x;
	getmaxyx(stdscr, max_y, max_x);

	int h = 18;
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

	if (show_help) {
		// Context-sensitive help
		bool in_pass = pass_editing;

		if (sidebar_active) {
			mvwprintw(win, row, 3, "-- Main Menu --");
			row++;
			line("UP / k", "Move up");
			line("DOWN / j", "Move down");
			line("Enter", "Select / Open");
			line("?", "Show this help");
			line("q", "Quit");
		} else if (login_creating) {
			mvwprintw(win, row, 3, "-- Create User --");
			row++;
			line("Tab", "Switch field");
			line("Enter", "Create / Next field");
			line("Backspace", "Delete char");
			line("ESC", "Cancel");
		} else if (login_editing) {
			mvwprintw(win, row, 3, "-- Enter Password --");
			row++;
			line("Enter", "Login");
			line("Backspace", "Delete char");
			line("ESC", "Cancel");
		} else if (current_view == MainView::Login) {
			mvwprintw(win, row, 3, "-- Select User --");
			row++;
			line("UP / k", "Move up");
			line("DOWN / j", "Move down");
			line("Enter", "Select / Login");
			line("d / D", "Delete user");
		} else if (in_pass) {
			mvwprintw(win, row, 3, "-- Change Password --");
			row++;
			line("Enter", "Save password");
			line("Backspace", "Delete char");
			line("ESC", "Cancel");
		} else if (current_view == MainView::MinerMenu) {
			mvwprintw(win, row, 3, "-- Miner --");
			row++;
			line("UP / k", "Move up");
			line("DOWN / j", "Move down");
			line("Enter", "Select action");
			line("c", "Collect gold");
			line("w / W", "Buy / Sell worker");
			line("l", "Level up");
			line("ESC / RIGHT", "Back to menu");
		} else if (current_view == MainView::Bag) {
			mvwprintw(win, row, 3, "-- Bag --");
			row++;
			line("UP / k", "Previous item");
			line("DOWN / j", "Next item");
			line("!", "Open terminal");
			line("ESC / RIGHT", "Back to menu");
			line("?", "Show this help");
		} else {
			mvwprintw(win, row, 3, "-- Menu --");
			row++;
			line("!", "Open terminal");
			line("ESC / RIGHT", "Back to sidebar");
			line("?", "Show this help");
		}
	}

	row++;
	mvwprintw(win, row, 3, "Press any key to close");

	help_color.disable(win);
	wnoutrefresh(win);
	doupdate();

	// Wait for any key
	wgetch(win);
	delwin(win);
}

// ============================================================
// Art loader
// ============================================================

int MainTui::load_art(const std::string& art_path, WINDOW* win, int start_y, int max_lines, int x_offset) {
	std::ifstream file(art_path);
	if (!file.is_open()) {
		art_color.enable(win);
		mvwprintw(win, start_y, x_offset, "[art not found]");
		art_color.disable(win);
		return start_y + 1;
	}

	int max_x = getmaxx(win);
	std::string line;
	int y = start_y;

	art_color.enable(win);
	while (std::getline(file, line) && y < start_y + max_lines) {
		if ((int)line.size() > max_x - x_offset - 2)
			line = line.substr(0, max_x - x_offset - 2);
		mvwprintw(win, y, x_offset, "%s", line.c_str());
		y++;
	}
	art_color.disable(win);
	return y;
}

// ============================================================
// Data
// ============================================================

void MainTui::load_bag() {
	if (username.empty()) return;

	std::string dir = "cache/" + username;
	std::filesystem::create_directories(dir);

	std::string path = dir + "/data.wh";
	bag.cache_get(path);

	if (!bag.contains_value("coins"))
		bag.set_value("coins", "100");

	coins = bag.get_value<int>("coins");
	build_bag_items();
}

void MainTui::save_bag() {
	if (username.empty()) return;

	std::string dir = "cache/" + username;
	std::filesystem::create_directories(dir);

	bag.set_value("coins", coins);
	bag.cache_put(dir + "/data.wh");
}

// ============================================================
// Actions
// ============================================================

void MainTui::do_login() {
	if (login_user.empty()) return;

	std::string path = "cache/" + login_user + "/data.wh";
	CacheData check;
	check.cache_get(path);

	if (check.contains_value("password")) {
		if (login_pass.empty()) return;
		uint64_t hash = random_seed(login_pass);
		uint64_t stored = std::stoull(check.get_value<std::string>("password"));
		if (hash != stored) {
			login_pass.clear();
			return;
		}
	}

	username = login_user;
	logged_in = true;
	load_bag();
	load_item_defs();
	build_menu();
	menu_selected = 0;
	sidebar_active = true;
	current_view = MainView::UserInfo;
	login_user.clear();
	login_pass.clear();
	login_field = 0;
	login_editing = false;
	login_creating = false;
	login_new_user.clear();
	login_user_selected = 0;
}

void MainTui::do_create_user() {
	if (login_new_user.empty()) return;

	// Check for invalid characters in username
	for (char c : login_new_user) {
		if (!std::isalnum(c) && c != '_' && c != '-') return;
	}

	std::string dir = "cache/" + login_new_user;
	std::string path = dir + "/data.wh";

	// Check if user already exists
	if (std::filesystem::exists(path)) {
		login_new_user.clear();
		login_pass.clear();
		login_field = 0;
		return;
	}

	std::filesystem::create_directories(dir);

	CacheData new_user;
	if (!login_pass.empty()) {
		uint64_t hash = random_seed(login_pass);
		new_user.set_value("password", std::to_string(hash));
	}
	new_user.set_value("coins", "100");
	new_user.cache_put(path);

	login_creating = false;
	login_new_user.clear();
	login_pass.clear();
	login_field = 0;
	curs_set(0);

	load_user_list();
}

void MainTui::do_delete_user() {
	if (login_user_list.empty()) return;
	if (login_user_selected <= 0 || login_user_selected > (int)login_user_list.size()) return;

	std::string name = login_user_list[login_user_selected - 1];
	std::string dir = "cache/" + name;

	std::filesystem::remove_all(dir);

	if (login_user_selected >= (int)login_user_list.size() && login_user_selected > 1)
		login_user_selected--;

	load_user_list();
}

void MainTui::do_change_password() {
	if (new_pass.empty()) return;

	uint64_t hash = random_seed(new_pass);
	bag.set_value("password", std::to_string(hash));
	save_bag();
	new_pass.clear();
	pass_editing = false;
	current_view = MainView::UserInfo;
	build_menu();
}

void MainTui::open_shop() {
	load_shop();
	ShopTui shop;
	shop.run(username);
	load_bag();
	build_bag_items();
	coins = bag.get_value<int>("coins");
}

void MainTui::do_logout() {
	save_bag();
	logged_in = false;
	username.clear();
	coins = 0;
	bag.clear();
	bag_items.clear();
	item_arts.clear();
	item_descriptions.clear();
	build_menu();
	menu_selected = 0;
	sidebar_active = true;
	current_view = MainView::Login;
	login_creating = false;
	login_editing = false;
	login_user.clear();
	login_pass.clear();
	login_new_user.clear();
	login_field = 0;
	load_user_list();
}

// ============================================================
// Input handling
// ============================================================

void MainTui::handle_input(int key) {
	// Help overlay captures all keys
	if (show_help) {
		show_help = false;
		return;
	}

	if (key == 'q' || key == 'Q') {
		if (logged_in) save_bag();
		running = false;
		return;
	}

	if (key == '?') {
		show_help = true;
		return;
	}

	if (login_creating) {
		handle_login_keys(key);
		return;
	}
	if (login_editing) {
		handle_login_keys(key);
		return;
	}
	if (pass_editing) {
		handle_password_keys(key);
		return;
	}

	if (cli_active && cli_editing) {
		handle_cli_keys(key);
		return;
	}

	if (sidebar_active) {
		handle_sidebar_keys(key);
	} else if (current_view == MainView::MinerMenu) {
		handle_miner_menu_keys(key);
	} else {
		handle_content_keys(key);
	}
}

void MainTui::handle_sidebar_keys(int key) {
	switch (key) {
		case KEY_UP:
		case 'k':
			menu_selected = (menu_selected + (int)menu_items.size() - 1) % (int)menu_items.size();
			break;
		case KEY_DOWN:
		case 'j':
			menu_selected = (menu_selected + 1) % (int)menu_items.size();
			break;
		case '\n':
		case KEY_ENTER: {
			if (menu_items.empty()) break;
			std::string label = menu_items[menu_selected].label;

			if (label == "Login") {
				current_view = MainView::Login;
				login_creating = false;
				login_editing = false;
				login_user.clear();
				login_pass.clear();
				login_new_user.clear();
				login_field = 0;
				load_user_list();
				sidebar_active = false;
				curs_set(0);
			} else if (label == "Shop") {
				open_shop();
			} else if (label == "Workers") {
				current_view = MainView::MinerMenu;
				miner_menu_selected = 0;
				miner_status_msg.clear();
				sidebar_active = false;
			} else if (label == "Logout") {
				do_logout();
			} else if (label == "Exit") {
				if (logged_in) save_bag();
				running = false;
			} else if (label == "Password") {
				current_view = MainView::Password;
				pass_editing = true;
				new_pass.clear();
				curs_set(1);
			} else {
				current_view = menu_items[menu_selected].view;
				sidebar_active = false;
				bag_selected = 0;
			}
			break;
		}
	}
}

void MainTui::handle_content_keys(int key) {
	// Toggle CLI panel with !
	if (key == '!') {
		if (cli_active && cli_editing) {
			cli_editing = false;
			curs_set(0);
		} else if (cli_active && !cli_editing) {
			cli_active = false;
		} else {
			cli_active = true;
			cli_editing = true;
			curs_set(1);
		}
		return;
	}

	// If CLI is active but not editing, Enter activates it
	if (cli_active && !cli_editing && (key == '\n' || key == KEY_ENTER)) {
		cli_editing = true;
		curs_set(1);
		return;
	}

	if (cli_active && !cli_editing) {
		// CLI panel visible but not editing, ignore most keys
		return;
	}

	// Route login keys when in Login view
	if (current_view == MainView::Login) {
		handle_login_keys(key);
		return;
	}

	switch (key) {
		case 27: // ESC
		case KEY_RIGHT:
			sidebar_active = true;
			break;

		case KEY_UP:
		case 'k':
			if (current_view == MainView::Bag && !bag_items.empty()) {
				bag_selected = (bag_selected + (int)bag_items.size() - 1) % (int)bag_items.size();
			}
			break;
		case KEY_DOWN:
		case 'j':
			if (current_view == MainView::Bag && !bag_items.empty()) {
				bag_selected = (bag_selected + 1) % (int)bag_items.size();
			}
			break;
	}
}

void MainTui::handle_login_keys(int key) {
	if (login_creating) {
		switch (key) {
			case '\t':
			case KEY_BTAB:
				login_field = (login_field + 1) % 2;
				break;
			case '\n':
			case KEY_ENTER:
				if (login_field == 1) {
					do_create_user();
					curs_set(0);
				} else {
					login_field = 1;
				}
				break;
			case 27: // ESC
				login_creating = false;
				login_new_user.clear();
				login_pass.clear();
				login_field = 0;
				curs_set(0);
				load_user_list();
				break;
			case KEY_BACKSPACE:
			case 127:
			case '\b':
				if (login_field == 0 && !login_new_user.empty())
					login_new_user.pop_back();
				else if (login_field == 1 && !login_pass.empty())
					login_pass.pop_back();
				break;
			default:
				if (key >= 32 && key < 127) {
					char c = (char)key;
					if (login_field == 0)
						login_new_user += c;
					else
						login_pass += c;
				}
				break;
		}
		return;
	}

	// Password entry mode (after selecting an existing user)
	if (login_editing) {
		switch (key) {
			case '\n':
			case KEY_ENTER:
				do_login();
				curs_set(0);
				break;
			case 27: // ESC
				login_editing = false;
				login_pass.clear();
				curs_set(0);
				break;
			case KEY_BACKSPACE:
			case 127:
			case '\b':
				if (!login_pass.empty())
					login_pass.pop_back();
				break;
			default:
				if (key >= 32 && key < 127) {
					login_pass += (char)key;
				}
				break;
		}
		return;
	}

	// User list browsing mode
	int total_items = 1 + (int)login_user_list.size();
	bool has_delete = !login_user_list.empty();
	if (has_delete) total_items++;

	switch (key) {
		case 27: // ESC
		case KEY_RIGHT:
			sidebar_active = true;
			build_menu();
			menu_selected = 0;
			break;
		case KEY_UP:
		case 'k':
			login_user_selected = (login_user_selected + total_items - 1) % total_items;
			break;
		case KEY_DOWN:
		case 'j':
			login_user_selected = (login_user_selected + 1) % total_items;
			break;
		case '\n':
		case KEY_ENTER: {
			if (total_items == 0) break;
			if (login_user_selected == 0) {
				// "+ Add User"
				login_creating = true;
				login_new_user.clear();
				login_pass.clear();
				login_field = 0;
				curs_set(1);
			} else if (login_user_selected <= (int)login_user_list.size()) {
				// Select existing user -> enter password
				login_user = login_user_list[login_user_selected - 1];
				// Check if user has a password
				std::string path = "cache/" + login_user + "/data.wh";
				CacheData check;
				check.cache_get(path);
				if (check.contains_value("password")) {
					login_editing = true;
					login_pass.clear();
					curs_set(1);
				} else {
					// No password, login directly
					do_login();
				}
			} else {
				// "x Delete User"
				do_delete_user();
			}
			break;
		}
		case 'd':
		case 'D':
			if (!login_user_list.empty() && login_user_selected > 0 &&
				login_user_selected <= (int)login_user_list.size()) {
				do_delete_user();
			}
			break;
	}
}

void MainTui::handle_password_keys(int key) {
	switch (key) {
		case '\n':
		case KEY_ENTER:
			do_change_password();
			curs_set(0);
			break;
		case 27: // ESC
			pass_editing = false;
			new_pass.clear();
			curs_set(0);
			current_view = MainView::Password;
			build_menu();
			menu_selected = 3;
			sidebar_active = true;
			break;
		case KEY_BACKSPACE:
		case 127:
		case '\b':
			if (!new_pass.empty())
				new_pass.pop_back();
			break;
		default:
			if (key >= 32 && key < 127) {
				new_pass += (char)key;
			}
			break;
	}
}

// ============================================================
// CLI Panel Input
// ============================================================

void MainTui::cli_execute(const std::string& line) {
	std::string output;

	// Handle built-in 'exit' to close CLI
	if (line == "exit" || line == "quit") {
		cli_active = false;
		cli_editing = false;
		curs_set(0);
		return;
	}

	// Handle 'help' built-in
	if (line == "help") {
		output = "  user <name>      - login or switch user\n";
		output += "  whoami           - show current user\n";
		output += "  password <pass>  - change password\n";
		output += "  add_item <n> [c] - add item to bag\n";
		output += "  add_coins <amt>  - add coins\n";
		output += "  shop             - open the shop\n";
		output += "  game             - open the miner\n";
		output += "  coins            - show coins\n";
		output += "  bag              - show bag\n";
		output += "  exit             - close terminal\n";
		cli_history.push_back({line, output});
		return;
	}

	// Parse tokens
	auto tokens = CommandLine::parse(line);
	if (tokens.empty()) {
		cli_history.push_back({line, ""});
		return;
	}

	std::string cmd = tokens[0];
	std::vector<std::string> args(tokens.begin() + 1, tokens.end());

	// Capture stdout via pipe
	int pipefd[2];
	int saved_stdout = dup(STDOUT_FILENO);
	int saved_stderr = dup(STDERR_FILENO);

	if (pipe(pipefd) == 0) {
		dup2(pipefd[1], STDOUT_FILENO);
		dup2(pipefd[1], STDERR_FILENO);
		close(pipefd[1]);

		// Execute command
		if (whiskey_cl.has(cmd)) {
			whiskey_cl.call(cmd, args);
		} else {
			std::cout << "Unknown command: " << cmd << std::endl;
			std::cout << "Type 'help' for available commands." << std::endl;
		}

		// Restore stdout/stderr
		fflush(stdout);
		fflush(stderr);
		dup2(saved_stdout, STDOUT_FILENO);
		dup2(saved_stderr, STDERR_FILENO);
		close(saved_stdout);
		close(saved_stderr);

		// Read captured output
		char buf[4096];
		ssize_t n;
		while ((n = read(pipefd[0], buf, sizeof(buf) - 1)) > 0) {
			buf[n] = '\0';
			output += buf;
		}
		close(pipefd[0]);
	} else {
		dup2(saved_stdout, STDOUT_FILENO);
		dup2(saved_stderr, STDERR_FILENO);
		close(saved_stdout);
		close(saved_stderr);
		output = "Error: failed to create pipe";
	}

	// Trim trailing newline
	while (!output.empty() && output.back() == '\n')
		output.pop_back();

	cli_history.push_back({line, output});
}

void MainTui::handle_cli_keys(int key) {
	switch (key) {
		case 27: // ESC - deactivate CLI
			cli_editing = false;
			curs_set(0);
			break;
		case '\n':
		case KEY_ENTER:
			if (!cli_input.empty()) {
				cli_execute(cli_input);
				cli_input.clear();
				cli_cursor = 0;
			}
			break;
		case KEY_BACKSPACE:
		case 127:
		case '\b':
			if (!cli_input.empty()) {
				cli_input.pop_back();
				cli_cursor--;
			}
			break;
		case KEY_LEFT:
			if (cli_cursor > 0)
				cli_cursor--;
			break;
		case KEY_RIGHT:
			if (cli_cursor < (int)cli_input.size())
				cli_cursor++;
			break;
		case KEY_UP:
			// Scroll history up
			break;
		case KEY_DOWN:
			// Scroll history down
			break;
		default:
			if (key >= 32 && key < 127) {
				cli_input += (char)key;
				cli_cursor++;
			}
			break;
	}
}

// ============================================================
// Run
// ============================================================

void MainTui::run() {
	tui_init();
	init_colors();
	setup_windows();
	build_menu();

	running = true;
	sidebar_active = true;
	current_view = MainView::Login;
	draw_all();

	while (running) {
		int key = wgetch(sidebar);
		handle_input(key);
		draw_all();
	}

	destroy_windows();
	tui_end();
}
