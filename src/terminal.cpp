#include "terminal.hpp"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>

#include <sys/ioctl.h>
#include <unistd.h>

namespace edinz {

Terminal::Terminal() {
    enableRawMode();
}

Terminal::~Terminal() {
    if (m_rawMode) {
        disableRawMode();
    }
}

void Terminal::enableRawMode() {
    if (tcgetattr(STDIN_FILENO, &m_origTermios) == -1) {
        throw std::runtime_error("tcgetattr failed");
    }

    termios raw = m_origTermios;

    // Input: no break, no CR-to-NL, no parity, no strip, no flow control
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    // Output: disable post-processing
    raw.c_oflag &= ~(OPOST);
    // Control: 8-bit chars
    raw.c_cflag |= CS8;
    // Local: no echo, no canonical, no signals, no extended
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);

    // Read returns after 1 byte, with 100ms timeout
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        throw std::runtime_error("tcsetattr failed");
    }

    m_rawMode = true;
}

void Terminal::disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &m_origTermios);
    m_rawMode = false;
}

static std::optional<char> readByte() {
    char c;
    auto n = ::read(STDIN_FILENO, &c, 1);
    if (n == 1) return c;
    return std::nullopt;
}

std::optional<KeyEvent> Terminal::readKey() {
    auto maybe = readByte();
    if (!maybe) return std::nullopt;

    char c = *maybe;

    // Escape sequences
    if (c == '\x1b') {
        auto seq1 = readByte();
        if (!seq1) return Key::Escape;

        if (*seq1 == '[') {
            auto seq2 = readByte();
            if (!seq2) return Key::Escape;

            // Arrow keys, Home, End: ESC [ A/B/C/D/H/F
            switch (*seq2) {
                case 'A': return Key::Up;
                case 'B': return Key::Down;
                case 'C': return Key::Right;
                case 'D': return Key::Left;
                case 'H': return Key::Home;
                case 'F': return Key::End;
                default: break;
            }

            // Extended sequences: ESC [ <number> ~
            if (*seq2 >= '0' && *seq2 <= '9') {
                auto seq3 = readByte();
                if (seq3 && *seq3 == '~') {
                    switch (*seq2) {
                        case '1': return Key::Home;
                        case '3': return Key::Delete;
                        case '4': return Key::End;
                        case '5': return Key::PageUp;
                        case '6': return Key::PageDown;
                        case '7': return Key::Home;
                        case '8': return Key::End;
                        default: break;
                    }
                }
            }

            return Key::Escape; // unrecognized sequence
        }

        if (*seq1 == 'O') {
            auto seq2 = readByte();
            if (!seq2) return Key::Escape;
            switch (*seq2) {
                case 'H': return Key::Home;
                case 'F': return Key::End;
                default: break;
            }
        }

        return Key::Escape;
    }

    // Ctrl keys: 1-26 map to Ctrl+A through Ctrl+Z
    if (c == '\r' || c == '\n') return Key::Enter;
    if (c == '\t') return Key::Tab;
    if (c == 127 || c == '\b') return Key::Backspace;

    if (c >= 1 && c <= 26) {
        return Ctrl{static_cast<char>('a' + c - 1)};
    }

    // Printable character
    return Char{c};
}

void Terminal::clear() {
    ::write(STDOUT_FILENO, "\x1b[2J\x1b[H", 7);
}

void Terminal::moveTo(int row, int col) {
    auto seq = std::string("\x1b[") + std::to_string(row) + ";" + std::to_string(col) + "H";
    ::write(STDOUT_FILENO, seq.data(), seq.size());
}

void Terminal::hideCursor() {
    ::write(STDOUT_FILENO, "\x1b[?25l", 6);
}

void Terminal::showCursor() {
    ::write(STDOUT_FILENO, "\x1b[?25h", 6);
}

void Terminal::write(std::string_view text) {
    ::write(STDOUT_FILENO, text.data(), text.size());
}

void Terminal::flush() {
    // Direct write via STDOUT_FILENO is unbuffered; nothing to flush.
    // Provided for API symmetry if buffering is added later.
}

std::pair<int, int> Terminal::size() const {
    winsize ws{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_row == 0) {
        return {24, 80}; // sensible fallback
    }
    return {ws.ws_row, ws.ws_col};
}

} // namespace edinz
