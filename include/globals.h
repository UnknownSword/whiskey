#pragma once

#include <cacheData.h>
#include <commandLine.h>
#include <utils/random.h>
#include <ui/shopTui.h>
#include <ui/minerTui.h>
#include <ctime>
#include <filesystem>
#include <algorithm>

// ==================== Global State ====================

inline string current_user = "";
inline CacheData bag;
inline CacheData shop;

// ==================== Command Lines ====================

inline CommandLine whiskey_cl("whiskey> ");
inline CommandLine shop_cl("shop> ");

// ==================== Helpers ====================

inline string today_string() {
	time_t now = time(nullptr);
	tm* t = localtime(&now);
	char buf[32];
	strftime(buf, sizeof(buf), "%Y-%m-%d", t);
	return string(buf);
}

inline string user_dir() {
	return "cache/" + current_user;
}

inline string user_file() {
	return user_dir() + "/data.wh";
}

inline string shop_file() {
	return "cache/shop/wh";
}

inline void load_items_dir(CacheData& items_def, const filesystem::path& dir) {
	if (!filesystem::exists(dir) || !filesystem::is_directory(dir)) return;
	for (auto& entry : filesystem::recursive_directory_iterator(dir)) {
		if (entry.is_regular_file() && entry.path().extension() == ".wh") {
			CacheData file_items;
			file_items.cache_get(entry.path().string());
			for (auto& [name, def] : file_items.childs) {
				items_def.childs[name] = def;
			}
		}
	}
}

inline bool str_to_int(const string& s, int& out) {
	try { out = stoi(s); return true; }
	catch (...) { return false; }
}

// ==================== Core ====================

inline void load_data() {
	if (current_user.empty()) return;
	filesystem::create_directories(user_dir());
	bag.cache_get(user_file());
	if (!bag.contains_value("coins"))
		bag.set_value("coins", "100");
}

inline void save_data() {
	if (current_user.empty()) return;
	filesystem::create_directories(user_dir());
	bag.cache_put(user_file());
}

inline void load_shop() {
	string today = today_string();

	shop.clear();
	shop.set_value("day", today);

	CacheData items_def;
	load_items_dir(items_def, "cache/items");

	uint64_t day_seed = random_seed(today);

	for (auto& [name, def] : items_def.childs) {
		int low = 1000, high = 2000;
		if (def.contains_child("price")) {
			CacheData& p = def.get_child("price");
			if (p.contains_value("low")) low = p.get_value<int>("low");
			if (p.contains_value("high")) high = p.get_value<int>("high");
		}

		uint64_t iseed = random_seed(name);
		int range = high - low;
		int p1 = low + (int)(random_seed(day_seed ^ iseed) % (range + 1));
		int p2 = low + (int)(random_seed(day_seed ^ iseed ^ 0xDEADBEEF) % (range + 1));

		int bye_price = max(p1, p2);
		int sell_price = min(p1, p2);
		if (bye_price < sell_price) swap(bye_price, sell_price);

		CacheData item;
		item.set_value("bye", bye_price);
		item.set_value("sell", sell_price);
		shop.childs[name] = item;
	}

	filesystem::create_directories("cache/shop");
	shop.cache_put(shop_file());
}

// ==================== Whiskey Commands ====================

inline uint64_t whiskey_login(vector<string> args) {
	if (args.empty()) {
		cout << "Usage: user <username> [password]" << endl;
		return 1;
	}

	string name = args[0];

	CacheData check;
	check.cache_get("cache/" + name + "/data.wh");

	if (check.contains_value("password")) {
		if (args.size() < 2) {
			cout << "Password required. Usage: user <name> <password>" << endl;
			return 1;
		}
		uint64_t hash = random_seed(args[1]);
		uint64_t stored = stoull(check.get_value<string>("password"));
		if (hash != stored) {
			cout << "Wrong password." << endl;
			return 1;
		}
	}

	save_data();
	current_user = name;
	load_data();
	cout << "Welcome, " << current_user << "!" << endl;
	cout << "Coins: " << bag.get_value<string>("coins") << endl;
	return 0;
}
inline uint64_t whiskey_whoami(vector<string> args){
	if (current_user.empty()) {
		cout << "No user logged in. Use: user <name>" << endl;
		return 1;
	}
	cout << "You are " << current_user << endl;
	return 0;
}

inline uint64_t whiskey_change_password(vector<string> args) {
	if (current_user.empty()) {
		cout << "No user logged in. Use: user <name>" << endl;
		return 1;
	}
	if (args.empty()) {
		cout << "Usage: password <new_password>" << endl;
		return 1;
	}
	uint64_t hash = random_seed(args[0]);
	bag.set_value("password", to_string(hash));
	save_data();
	cout << "Password changed." << endl;
	return 0;
}

inline uint64_t whiskey_add_item(vector<string> args) {
	if (current_user.empty()) {
		cout << "No user logged in. Use: user <name>" << endl;
		return 1;
	}
	if (args.empty()) {
		cout << "Usage: add_item <item_name> [count]" << endl;
		return 1;
	}
	string item = args[0];
	int count = 1;
	if (args.size() >= 2) {
		if (!str_to_int(args[1], count) || count <= 0) {
			cout << "Invalid count." << endl;
			return 1;
		}
	}

	int current = 0;
	if (bag.contains_child(item) && bag.get_child(item).contains_value("count"))
		current = bag.get_child(item).get_value<int>("count");

	bag.childs[item].set_value("count", current + count);
	save_data();
	cout << "Added " << count << "x " << item << " (total: " << (current + count) << ")" << endl;
	return 0;
}

inline uint64_t whiskey_add_coins(vector<string> args) {
	if (current_user.empty()) {
		cout << "No user logged in. Use: user <name>" << endl;
		return 1;
	}
	if (args.empty()) {
		cout << "Usage: add_coins <amount>" << endl;
		return 1;
	}
	int amount = 0;
	if (!str_to_int(args[0], amount) || amount <= 0) {
		cout << "Invalid amount." << endl;
		return 1;
	}
	int coins = bag.get_value<int>("coins");
	bag.set_value("coins", coins + amount);
	save_data();
	cout << "Added " << amount << " coins. Total: " << (coins + amount) << endl;
	return 0;
}

inline uint64_t whiskey_open_shop(vector<string> args) {
	if (current_user.empty()) {
		cout << "No user logged in. Use: user <name>" << endl;
		return 1;
	}
	load_shop();
	ShopTui shop;
	shop.run(current_user);
	return 0;
}

inline uint64_t whiskey_open_game(vector<string> args) {
	if (current_user.empty()) {
		cout << "No user logged in. Use: user <name>" << endl;
		return 1;
	}
	MinerTui miner;
	miner.run(current_user);
	return 0;
}

inline uint64_t whiskey_show_coins(vector<string> args) {
	if (current_user.empty()) {
		cout << "No user logged in." << endl;
		return 1;
	}
	cout << "Coins: " << bag.get_value<string>("coins") << endl;
	return 0;
}

inline uint64_t whiskey_show_bag(vector<string> args) {
	if (current_user.empty()) {
		cout << "No user logged in." << endl;
		return 1;
	}
	cout << "=== Bag (" << current_user << ") ===" << endl;
	cout << "  coins: " << bag.get_value<string>("coins") << endl;
	for (auto& [name, node] : bag.childs) {
		if (name == "password") continue;
		if (node.contains_value("count"))
			cout << "  " << name << " x" << node.get_value<string>("count") << endl;
	}
	return 0;
}

// ==================== Shop Commands ====================

inline uint64_t shop_show(vector<string> args) {
	cout << "=== Daily Shop (" << shop.get_value<string>("day") << ") ===" << endl;
	printf("  %-12s %8s %8s\n", "Item", "Bye", "Sell");
	for (auto& [name, node] : shop.childs) {
		if (name == "day") continue;
		int bye = node.contains_value("bye") ? node.get_value<int>("bye") : 0;
		int sell = node.contains_value("sell") ? node.get_value<int>("sell") : 0;
		printf("  %-12s %8d %8d\n", name.c_str(), bye, sell);
	}
	return 0;
}

inline uint64_t shop_show_bag(vector<string> args) {
	return whiskey_show_bag(args);
}

inline uint64_t shop_show_coins(vector<string> args) {
	return whiskey_show_coins(args);
}

inline uint64_t shop_bye_items(vector<string> args) {
	if (current_user.empty()) {
		cout << "No user logged in." << endl;
		return 1;
	}
	if (args.empty()) {
		cout << "Usage: bye <item> [count]" << endl;
		return 1;
	}

	string item = args[0];
	int count = 1;
	if (args.size() >= 2) {
		if (!str_to_int(args[1], count) || count <= 0) {
			cout << "Invalid count." << endl;
			return 1;
		}
	}

	if (!shop.contains_child(item)) {
		cout << "Item '" << item << "' not in shop." << endl;
		return 1;
	}

	CacheData& shop_item = shop.get_child(item);
	int bye_price = shop_item.get_value<int>("bye");
	int total = bye_price * count;
	int coins = bag.get_value<int>("coins");

	if (total > coins) {
		cout << "Not enough coins. Need " << total << ", have " << coins << "." << endl;
		return 1;
	}

	bag.set_value("coins", coins - total);

	int current = 0;
	if (bag.contains_child(item) && bag.get_child(item).contains_value("count"))
		current = bag.get_child(item).get_value<int>("count");
	bag.childs[item].set_value("count", current + count);

	save_data();
	cout << "Bought " << count << "x " << item << " for " << total << " coins." << endl;
	cout << "Remaining coins: " << (coins - total) << endl;
	return 0;
}

inline uint64_t shop_sell_items(vector<string> args) {
	if (current_user.empty()) {
		cout << "No user logged in." << endl;
		return 1;
	}
	if (args.empty()) {
		cout << "Usage: sell <item> [count]" << endl;
		return 1;
	}

	string item = args[0];
	int count = 1;
	if (args.size() >= 2) {
		if (!str_to_int(args[1], count) || count <= 0) {
			cout << "Invalid count." << endl;
			return 1;
		}
	}

	if (!shop.contains_child(item)) {
		cout << "Item '" << item << "' not in shop." << endl;
		return 1;
	}

	if (!bag.contains_child(item) || !bag.get_child(item).contains_value("count")) {
		cout << "You don't have any " << item << "." << endl;
		return 1;
	}

	int have = bag.get_child(item).get_value<int>("count");

	if (count > have) {
		cout << "You only have " << have << "x " << item << "." << endl;
		return 1;
	}

	int sell_price = shop.get_child(item).get_value<int>("sell");
	int total = sell_price * count;
	int coins = bag.get_value<int>("coins");

	if (have == count)
		bag.childs.erase(item);
	else
		bag.childs[item].set_value("count", have - count);

	bag.set_value("coins", coins + total);

	save_data();
	cout << "Sold " << count << "x " << item << " for " << total << " coins." << endl;
	cout << "Total coins: " << (coins + total) << endl;
	return 0;
}

inline uint64_t shop_back(vector<string> args) {
	shop_cl.stop();
	return 0;
}

// ==================== Init ====================

inline void global_init() {
	whiskey_cl
		.add("user", whiskey_login, "login or switch user")
		.add("whoami", whiskey_whoami, "show the username")
		.add("password", whiskey_change_password, "change password")
		.add("add_item", whiskey_add_item, "add item to bag")
		.add("add_coins", whiskey_add_coins, "add coins to wallet")
		.add("shop", whiskey_open_shop, "open the shop")
		.add("game", whiskey_open_game, "open the game")
		.add("coins", whiskey_show_coins, "show your coins")
		.add("bag", whiskey_show_bag, "show your bag");

	shop_cl
		.add("show", shop_show, "show shop items and prices")
		.add("bye", shop_bye_items, "buy an item from shop")
		.add("sell", shop_sell_items, "sell an item to shop")
		.add("bag", shop_show_bag, "show your bag")
		.add("coins", shop_show_coins, "show your coins")
		.add("back", shop_back, "go back to main menu");
}
