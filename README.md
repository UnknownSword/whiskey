# Whiskey

A terminal-based fantasy shop and idle mining game built with C++20 and ncurses.

Trade 14 fantasy items in a daily-rotating shop, manage your inventory, and run a gold mine with workers. Features a full TUI interface with colored panels, ASCII art, and keyboard navigation.

## Features

- **Fantasy Shop** - Buy and sell 14 items with daily-changing prices
- **Inventory System** - Collect items with lore descriptions and ASCII art
- **Idle Miner** - Hire workers, level up, and passively generate gold in real time
- **Embedded Terminal** - Run commands directly inside the TUI
- **User System** - Multiple users with password support and persistent saves
- **Custom File Format** - `.wh` files for data persistence

## Requirements

- C++20 compiler (GCC 10+, Clang 12+, MSVC 2019+)
- GNU Make
- ncursesw library (wide-character support)

## Build & Run

### Arch Linux

```sh
sudo pacman -S gcc make ncurses
make
./bin/whiskey
```

### Debian / Ubuntu

```sh
sudo apt install g++ make libncursesw5-dev
make
./bin/whiskey
```

### Fedora

```sh
sudo dnf install gcc-c++ make ncurses-devel
make
./bin/whiskey
```

### openSUSE

```sh
sudo zypper install gcc-c++ make libncursesw-devel
make
./bin/whiskey
```

### Void Linux

```sh
sudo xbps-install -S gcc make ncurses-devel
make
./bin/whiskey
```

### macOS (Homebrew)

```sh
brew install gcc make ncurses
make
./bin/whiskey
```

### Windows (MSYS2 / MinGW)

```sh
# Install MSYS2 from https://www.msys2.org
# Open MSYS2 MinGW 64-bit terminal:
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-make mingw-w64-x86_64-ncurses

mingw32-make
bin\whiskey.exe
```

### Debug build

```sh
make safe
./bin/whiskey
```

## Controls

| Key | Context | Action |
|-----|---------|--------|
| Arrow keys / j/k | Sidebar | Navigate menu |
| Enter | Sidebar | Select item |
| `!` | Main | Toggle embedded terminal |
| `?` | Any | Show help |

### Shop

| Key | Action |
|-----|--------|
| j/k | Navigate items |
| b / B | Buy 1 / 5 |
| s / S | Sell 1 / 5 |
| q | Exit shop |

### Miner

| Key | Action |
|-----|--------|
| c | Collect gold |
| w | Buy worker |
| W | Sell worker |
| l | Level up |
| q | Exit miner |

## Embedded Terminal Commands

```
user <name> [password]   - Login or switch user
whoami                   - Show current user
password <new_pass>      - Change password
add_item <item> [count]  - Add item to bag
add_coins <amount>       - Add coins
shop                     - Open the shop
game                     - Open the miner
coins                    - Show coins
bag                      - Show inventory
```

## Project Structure

```
whiskey/
├── main.cpp                 # Entry point
├── Makefile                 # GNU Make build
├── include/
│   ├── cacheData.h          # Custom .wh file parser
│   ├── commandLine.h        # CLI command dispatcher
│   ├── globals.h            # Global state and commands
│   └── ui/                  # TUI headers
├── src/ui/
│   ├── mainTui.cpp          # Main application shell
│   ├── shopTui.cpp          # Shop interface
│   ├── minerTui.cpp         # Miner game
│   └── utils/               # Colors, menus, ncurses helpers
├── assets/
│   ├── arts/items/          # ASCII art for items
│   └── descriptions/        # Lore text for items
└── cache/
    ├── items/               # Item definitions (.wh)
    └── shop/                # Daily shop prices (generated)
```

## License

No license specified.
=======
# whiskey
basic game for testing my offline daily shop algorithm
>>>>>>> f738455d303404018a44b72f43a8538ec8c339e7
=======
# whiskey
basic game for testing my offline daily shop algorithm
>>>>>>> f738455d303404018a44b72f43a8538ec8c339e7
