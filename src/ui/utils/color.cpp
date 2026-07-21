#include <ui/utils/color.h>

short next_custom_color = 16;

Color::Color() : r(0), g(0), b(0) {}

Color::Color(int rgb) 
    : r((rgb >> 16) & 0xFF), 
      g((rgb >> 8) & 0xFF), 
      b(rgb & 0xFF) {}

Color::Color(short red, short green, short blue) 
    : r(red), g(green), b(blue) {
    if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) {
        throw std::invalid_argument("Color components must be in range 0-255");
    }
}

short Color::nc_r() const { return static_cast<short>(r * 1000LL / 255); }
short Color::nc_g() const { return static_cast<short>(g * 1000LL / 255); }
short Color::nc_b() const { return static_cast<short>(b * 1000LL / 255); }

int Color::to_rgb() const {
    return (static_cast<int>(r) << 16) | (static_cast<int>(g) << 8) | b;
}

bool Color::operator==(const Color& other) const {
    return r == other.r && g == other.g && b == other.b;
}

bool Color::operator!=(const Color& other) const {
    return !(*this == other);
}

// --------------------------------------------------

short ScreenColor::get_or_allocate_color(const Color& c) {
    if (next_custom_color >= COLORS) {
        return COLOR_WHITE;
    }
    init_color(next_custom_color, c.nc_r(), c.nc_g(), c.nc_b());
    return next_custom_color++;
}

void ScreenColor::initialize() {
    if (!has_colors()) {
        throw std::runtime_error("Terminal does not support colors");
    }
    if (start_color() == ERR) {
        throw std::runtime_error("Failed to start colors");
    }

    short fg_idx = get_or_allocate_color(fg);
    short bg_idx = get_or_allocate_color(bg);

    pair_id = alloc_pair(fg_idx, bg_idx);
    if (pair_id == -1) {
        throw std::runtime_error("Failed to allocate color pair");
    }
    allocated = true;
}

// --------------------------------------------------

ScreenColor::ScreenColor() 
    : pair_id(0), attrs(A_NORMAL), fg(), bg(), allocated(false) {}

ScreenColor::ScreenColor(const Color& foreground, const Color& background, attr_t attributes)
    : pair_id(0), attrs(attributes), fg(foreground), bg(background), allocated(false) {
    initialize();
}

ScreenColor::ScreenColor(int fg_rgb, int bg_rgb, attr_t attributes)
    : ScreenColor(Color(fg_rgb), Color(bg_rgb), attributes) {}

ScreenColor::ScreenColor(const ScreenColor& other)
    : pair_id(0), attrs(other.attrs), fg(other.fg), bg(other.bg), allocated(false) {
    if (other.allocated && other.pair_id > 0) {
        int fg_idx = get_or_allocate_color(fg);
        int bg_idx = get_or_allocate_color(bg);
        pair_id = alloc_pair(fg_idx, bg_idx);
        allocated = (pair_id > 0);
    }
}

ScreenColor::ScreenColor(ScreenColor&& other) noexcept
    : pair_id(other.pair_id), attrs(other.attrs), 
      fg(other.fg), bg(other.bg), allocated(other.allocated) {
    other.pair_id = 0;
    other.allocated = false;
}

ScreenColor::~ScreenColor() {
    release();
}

ScreenColor& ScreenColor::operator=(const ScreenColor& other) {
    if (this != &other) {
        release();
        attrs = other.attrs;
        fg = other.fg;
        bg = other.bg;
        if (other.allocated && other.pair_id > 0) {
            int fg_idx = get_or_allocate_color(fg);
            int bg_idx = get_or_allocate_color(bg);
            pair_id = alloc_pair(fg_idx, bg_idx);
            allocated = (pair_id > 0);
        }
    }
    return *this;
}

ScreenColor& ScreenColor::operator=(ScreenColor&& other) noexcept {
    if (this != &other) {
        release();
        pair_id = other.pair_id;
        attrs = other.attrs;
        fg = other.fg;
        bg = other.bg;
        allocated = other.allocated;
        other.pair_id = 0;
        other.allocated = false;
    }
    return *this;
}

attr_t ScreenColor::get() const {
    if (pair_id <= 0) return attrs;
    return COLOR_PAIR(pair_id) | attrs;
}

void ScreenColor::enable(WINDOW* window) const {
    wattron(window, get());
}

void ScreenColor::disable(WINDOW* window) const {
    wattroff(window, get());
}

void ScreenColor::release() {
    if (allocated && pair_id > 0) {
        free_pair(pair_id);
    }
    pair_id = 0;
    allocated = false;
}

int ScreenColor::get_pair_id() const { return pair_id; }
attr_t ScreenColor::get_attrs() const { return attrs; }
void ScreenColor::set_attrs(attr_t a) { attrs = a; }

// --------------------------------------------------

void reset_colors() {
    for (int i = 1; i < COLOR_PAIRS; ++i) {
        free_pair(i);
    }
    next_custom_color = 16;
}
