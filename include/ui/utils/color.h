#pragma once

#include <curses.h>
#include <cstdint>
#include <stdexcept>

/**
 * Represents an RGB color (0-255 per component).
 */
struct Color {
    short r;
    short g;
    short b;

    Color();
    explicit Color(int rgb);
    Color(short red, short green, short blue);

    short nc_r() const;
    short nc_g() const;
    short nc_b() const;
    int to_rgb() const;

    bool operator==(const Color& other) const;
    bool operator!=(const Color& other) const;
};

/**
 * Global counter for custom color slots.
 */
extern short next_custom_color;

/**
 * Reset all custom colors and pairs.
 * Call this to start fresh with color allocation.
 */
void reset_colors();

/**
 * Manages a color pair with automatic allocation.
 */
struct ScreenColor {
    int pair_id;
    attr_t attrs;
    Color fg;
    Color bg;
    bool allocated;

    ScreenColor();
    explicit ScreenColor(const Color& foreground, const Color& background, attr_t attributes = A_NORMAL);
    ScreenColor(int fg_rgb, int bg_rgb, attr_t attributes = A_NORMAL);
    ScreenColor(const ScreenColor& other);
    ScreenColor(ScreenColor&& other) noexcept;
    ~ScreenColor();

    ScreenColor& operator=(const ScreenColor& other);
    ScreenColor& operator=(ScreenColor&& other) noexcept;

    attr_t get() const;
    void enable(WINDOW* = stdscr) const;
    void disable(WINDOW* = stdscr) const;
    void release();

    int get_pair_id() const;
    attr_t get_attrs() const;
    void set_attrs(attr_t a);

    // Public helpers
    static short get_or_allocate_color(const Color& c);
    void initialize();
};

