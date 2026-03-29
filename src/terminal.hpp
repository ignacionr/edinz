#pragma once

#include <cstdint>
#include <optional>
#include <string_view>
#include <variant>

#include <termios.h>

namespace edinz {

// Named keys (non-printable)
enum class Key : std::uint8_t {
    Up,
    Down,
    Left,
    Right,
    Enter,
    Escape,
    Backspace,
    Tab,
    Home,
    End,
    PageUp,
    PageDown,
    Delete,
};

// A printable character
struct Char {
    char ch;
};

// Ctrl+<letter>
struct Ctrl {
    char ch; // lowercase letter, e.g. 'q' for Ctrl+Q
};

using KeyEvent = std::variant<Key, Char, Ctrl>;

// RAII wrapper around termios raw mode
class Terminal {
public:
    Terminal();
    ~Terminal();

    Terminal(const Terminal&) = delete;
    Terminal& operator=(const Terminal&) = delete;

    // Read a single key event (blocks until input available)
    std::optional<KeyEvent> readKey();

    // Screen operations
    void clear();
    void moveTo(int row, int col);
    void hideCursor();
    void showCursor();
    void write(std::string_view text);
    void flush();

    // Terminal dimensions (rows, cols)
    std::pair<int, int> size() const;

private:
    void enableRawMode();
    void disableRawMode();

    termios m_origTermios{};
    bool m_rawMode{false};
};

} // namespace edinz
