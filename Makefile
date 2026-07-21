.PHONY: all build run clean safe install uninstall

CXX      ?= g++
CXXFLAGS := -std=c++20 -Iinclude -O2
LIBS     := -lncursesw

SRC_DIR   := src
BUILD_DIR := build
BIN_DIR   := bin
PREFIX    ?= /usr/local

MAIN_SRC  := main
BINARY    := $(BIN_DIR)/whiskey

SRCS := $(shell find $(SRC_DIR) -name '*.cpp')
OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS)) \
        $(BUILD_DIR)/$(MAIN_SRC).o

all: build

build: $(BINARY)

$(BINARY): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(OBJS) $(CXXFLAGS) $(LIBS) -o $@
	@echo "Build complete: $@"

$(BUILD_DIR)/$(MAIN_SRC).o: $(MAIN_SRC).cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: build
	./$(BINARY)

safe: CXXFLAGS += -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined -g -O0
safe: build

install: build
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(BINARY) $(DESTDIR)$(PREFIX)/bin/whiskey
	@echo "Installed to $(DESTDIR)$(PREFIX)/bin/whiskey"
	@echo "Note: Run 'whiskey' from the project directory (assets and cache are required)."

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/whiskey

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR) tmp
