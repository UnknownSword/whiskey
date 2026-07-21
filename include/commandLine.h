#pragma once

#include <string>
#include <map>
#include <vector>
#include <cstdint>
#include <iostream>
#include <cctype>
#include <algorithm>

using CommandLineFuncArg = std::vector<std::string>;
using CommandLineFuncReturn = uint64_t;
using CommandLineFunc = uint64_t (*)(std::vector<std::string>);

struct CommandLineEntry {
	CommandLineFunc func;
	std::string desc;
};

struct CommandLine {
	std::map<std::string, CommandLineEntry> commands;
	uint64_t last_status = 0;
	std::string prompt = "-> ";
	bool running = false;

	CommandLine() = default;
	explicit CommandLine(const std::string& p) : prompt(p) {}

	CommandLine& add(const std::string& name, CommandLineFunc func, const std::string& desc = "") {
		commands[name] = { func, desc };
		return *this;
	}

	CommandLine& rem(const std::string& name) {
		commands.erase(name);
		return *this;
	}

	bool has(const std::string& name) const {
		return commands.contains(name);
	}

	uint64_t call(const std::string& name, const std::vector<std::string>& args) {
		auto it = commands.find(name);
		if (it == commands.end()) {
			std::cerr << "Unknown command: " << name << std::endl;
			last_status = UINT64_MAX;
			return UINT64_MAX;
		}
		last_status = it->second.func(args);
		return last_status;
	}

	uint64_t call(const std::vector<std::string>& tokens) {
		if (tokens.empty()) {
			last_status = 0;
			return 0;
		}
		return call(tokens[0], { tokens.begin() + 1, tokens.end() });
	}

	void stop() { running = false; }

	void print_help() const {
		std::cerr << "  exit";
		std::cerr << " - exit the command line" << std::endl;
		for (auto& [name, entry] : commands) {
			std::cerr << "  " << name;
			if (!entry.desc.empty())
				std::cerr << " - " << entry.desc;
			std::cerr << std::endl;
		}
	}

	void run() {
		running = true;
		while (running) {
			std::string line;
			std::cerr << prompt;
			if (!std::getline(std::cin, line))
				break;

			std::vector<std::string> tokens = parse(line);
			if (tokens.empty()) continue;

			if (tokens[0] == "help") {
				print_help();
				continue;
			}
			if (tokens[0] == "quit" || tokens[0] == "exit")
				break;

			call(tokens);
		}
		running = false;
	}

	void run_once() {
		std::string line;
		std::cerr << prompt;
		if (!std::getline(std::cin, line))
			return;
		std::vector<std::string> tokens = parse(line);
		if (!tokens.empty())
			call(tokens);
	}

	static std::vector<std::string> parse(const std::string& s) {
		std::vector<std::string> result;
		std::string token;
		size_t i = 0;
		size_t len = s.size();

		auto skip_ws = [&]() {
			while (i < len && std::isspace(static_cast<unsigned char>(s[i])))
				i++;
		};

		skip_ws();
		while (i < len) {
			if (s[i] == '"') {
				i++;
				token.clear();
				while (i < len && s[i] != '"')
					token += s[i++];
				if (i < len) i++;
				result.push_back(std::move(token));
			} else {
				token.clear();
				while (i < len && !std::isspace(static_cast<unsigned char>(s[i])) && s[i] != '"')
					token += s[i++];
				result.push_back(std::move(token));
			}
			skip_ws();
		}
		return result;
	}
};
