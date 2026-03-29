#include "window.hpp"

#include <algorithm>
#include <format>
#include <string>

namespace edinz {

namespace {

std::string toUtf8(char32_t cp) {
    std::string result;
    if (cp < 0x80) {
        result += static_cast<char>(cp);
    } else if (cp < 0x800) {
        result += static_cast<char>(0xC0 | (cp >> 6));
        result += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        result += static_cast<char>(0xE0 | (cp >> 12));
        result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        result += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        result += static_cast<char>(0xF0 | (cp >> 18));
        result += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        result += static_cast<char>(0x80 | (cp & 0x3F));
    }
    return result;
}

// Single-line box-drawing
constexpr char32_t kSingleTL = U'\u250C'; // ┌
constexpr char32_t kSingleTR = U'\u2510'; // ┐
constexpr char32_t kSingleBL = U'\u2514'; // └
constexpr char32_t kSingleBR = U'\u2518'; // ┘
constexpr char32_t kSingleHz = U'\u2500'; // ─
constexpr char32_t kSingleVt = U'\u2502'; // │

// Double-line box-drawing (used for focused windows)
constexpr char32_t kDoubleTL = U'\u2554'; // ╔
constexpr char32_t kDoubleTR = U'\u2557'; // ╗
constexpr char32_t kDoubleBL = U'\u255A'; // ╚
constexpr char32_t kDoubleBR = U'\u255D'; // ╝
constexpr char32_t kDoubleHz = U'\u2550'; // ═
constexpr char32_t kDoubleVt = U'\u2551'; // ║

} // namespace

// ── ScreenBuffer ──────────────────────────────────────────

ScreenBuffer::ScreenBuffer(int rows, int cols)
    : m_rows(rows), m_cols(cols)
    , m_cells(static_cast<std::size_t>(rows),
              std::vector<Cell>(static_cast<std::size_t>(cols)))
{}

Cell& ScreenBuffer::at(int row, int col) {
    return m_cells[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)];
}

const Cell& ScreenBuffer::at(int row, int col) const {
    return m_cells[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)];
}

void ScreenBuffer::writeText(int row, int col, std::string_view text, uint8_t attr) {
    if (row < 0 || row >= m_rows) return;
    int c = col;
    for (char ch : text) {
        if (c < 0) { ++c; continue; }
        if (c >= m_cols) break;
        auto& cell = m_cells[static_cast<std::size_t>(row)][static_cast<std::size_t>(c)];
        cell.ch = static_cast<char32_t>(static_cast<unsigned char>(ch));
        cell.attr = attr;
        ++c;
    }
}

void ScreenBuffer::drawBox(int row, int col, int height, int width,
                           bool doubleLine, uint8_t attr) {
    if (height < 2 || width < 2) return;

    auto tl = doubleLine ? kDoubleTL : kSingleTL;
    auto tr = doubleLine ? kDoubleTR : kSingleTR;
    auto bl = doubleLine ? kDoubleBL : kSingleBL;
    auto br = doubleLine ? kDoubleBR : kSingleBR;
    auto hz = doubleLine ? kDoubleHz : kSingleHz;
    auto vt = doubleLine ? kDoubleVt : kSingleVt;

    auto set = [&](int r, int c, char32_t ch) {
        if (r >= 0 && r < m_rows && c >= 0 && c < m_cols) {
            m_cells[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)] = {ch, attr};
        }
    };

    // Corners
    set(row, col, tl);
    set(row, col + width - 1, tr);
    set(row + height - 1, col, bl);
    set(row + height - 1, col + width - 1, br);

    // Horizontal edges
    for (int c = col + 1; c < col + width - 1; ++c) {
        set(row, c, hz);
        set(row + height - 1, c, hz);
    }

    // Vertical edges
    for (int r = row + 1; r < row + height - 1; ++r) {
        set(r, col, vt);
        set(r, col + width - 1, vt);
    }
}

void ScreenBuffer::fillRect(int row, int col, int height, int width,
                            char32_t ch, uint8_t attr) {
    for (int r = row; r < row + height; ++r) {
        if (r < 0 || r >= m_rows) continue;
        for (int c = col; c < col + width; ++c) {
            if (c < 0 || c >= m_cols) continue;
            m_cells[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)] = {ch, attr};
        }
    }
}

void ScreenBuffer::renderTo(Terminal& term) const {
    std::string output;
    output.reserve(static_cast<std::size_t>(m_rows * m_cols * 4));

    uint8_t currentAttr = 0;

    for (int r = 0; r < m_rows; ++r) {
        output += std::format("\x1b[{};1H", r + 1);

        for (int c = 0; c < m_cols; ++c) {
            const auto& cell =
                m_cells[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)];

            if (cell.attr != currentAttr) {
                output += "\x1b[0m";
                if (cell.attr & Cell::Bold)    output += "\x1b[1m";
                if (cell.attr & Cell::Dim)     output += "\x1b[2m";
                if (cell.attr & Cell::Reverse) output += "\x1b[7m";
                currentAttr = cell.attr;
            }

            output += toUtf8(cell.ch);
        }
    }

    if (currentAttr != 0) {
        output += "\x1b[0m";
    }

    term.write(output);
}

// ── Window ────────────────────────────────────────────────

Window::Window(int row, int col, int height, int width, std::string title)
    : m_row(row), m_col(col), m_height(height), m_width(width)
    , m_title(std::move(title))
{}

void Window::move(int row, int col) {
    m_row = row;
    m_col = col;
}

void Window::resize(int height, int width) {
    m_height = height;
    m_width = width;
}

void Window::setContent(std::vector<std::string> lines) {
    m_content = std::move(lines);
    m_scrollOffset = 0;
}

void Window::addLine(const std::string& line) {
    m_content.push_back(line);
}

void Window::clearContent() {
    m_content.clear();
    m_scrollOffset = 0;
}

void Window::scrollUp() {
    if (m_scrollOffset > 0) --m_scrollOffset;
}

void Window::scrollDown() {
    int contentHeight = m_height - 2;
    int maxScroll = std::max(0, static_cast<int>(m_content.size()) - contentHeight);
    if (m_scrollOffset < maxScroll) ++m_scrollOffset;
}

void Window::renderTo(ScreenBuffer& buf) const {
    if (!m_visible) return;

    // Fill window background
    buf.fillRect(m_row, m_col, m_height, m_width);

    // Border: double line for focused, single for unfocused
    buf.drawBox(m_row, m_col, m_height, m_width, m_focused);

    // Title in top border
    if (!m_title.empty()) {
        int maxLen = m_width - 4;
        if (maxLen > 0) {
            auto titleText = m_title.substr(
                0, std::min(m_title.size(), static_cast<std::size_t>(maxLen)));
            std::string display = " " + titleText + " ";
            uint8_t attr = m_focused ? Cell::Bold : 0;
            buf.writeText(m_row, m_col + 2, display, attr);
        }
    }

    // Content area (inside border with 1-char padding on each side)
    int contentTop    = m_row + 1;
    int contentLeft   = m_col + 2;
    int contentWidth  = m_width - 4;
    int contentHeight = m_height - 2;

    for (int i = 0; i < contentHeight; ++i) {
        int lineIdx = m_scrollOffset + i;
        if (lineIdx < 0 || lineIdx >= static_cast<int>(m_content.size())) continue;
        const auto& line = m_content[static_cast<std::size_t>(lineIdx)];
        int len = std::min(static_cast<int>(line.size()), contentWidth);
        for (int j = 0; j < len; ++j) {
            int r = contentTop + i;
            int c = contentLeft + j;
            if (r >= 0 && r < buf.rows() && c >= 0 && c < buf.cols()) {
                buf.at(r, c).ch =
                    static_cast<char32_t>(static_cast<unsigned char>(
                        line[static_cast<std::size_t>(j)]));
            }
        }
    }

    // Scroll indicators
    if (m_scrollOffset > 0) {
        int r = m_row;
        int c = m_col + m_width - 2;
        if (r >= 0 && r < buf.rows() && c >= 0 && c < buf.cols()) {
            buf.at(r, c).ch = U'\u25B2'; // ▲
        }
    }

    int maxScroll = std::max(0, static_cast<int>(m_content.size()) - contentHeight);
    if (m_scrollOffset < maxScroll) {
        int r = m_row + m_height - 1;
        int c = m_col + m_width - 2;
        if (r >= 0 && r < buf.rows() && c >= 0 && c < buf.cols()) {
            buf.at(r, c).ch = U'\u25BC'; // ▼
        }
    }
}

// ── WindowManager ─────────────────────────────────────────

WindowManager::WindowPtr WindowManager::createWindow(
    int row, int col, int height, int width, const std::string& title)
{
    auto win = std::make_shared<Window>(row, col, height, width, title);
    m_windows.push_back(win);
    m_focusIndex = static_cast<int>(m_windows.size()) - 1;
    updateFocusFlags();
    return win;
}

void WindowManager::closeWindow(const WindowPtr& win) {
    auto it = std::find(m_windows.begin(), m_windows.end(), win);
    if (it == m_windows.end()) return;

    int idx = static_cast<int>(it - m_windows.begin());
    m_windows.erase(it);

    if (m_windows.empty()) {
        m_focusIndex = -1;
    } else if (m_focusIndex >= static_cast<int>(m_windows.size())) {
        m_focusIndex = static_cast<int>(m_windows.size()) - 1;
    } else if (idx < m_focusIndex) {
        --m_focusIndex;
    }
    updateFocusFlags();
}

void WindowManager::closeFocused() {
    if (auto win = focusedWindow()) {
        closeWindow(win);
    }
}

void WindowManager::focusNext() {
    if (m_windows.empty()) return;
    ++m_focusIndex;
    if (m_focusIndex >= static_cast<int>(m_windows.size())) {
        m_focusIndex = -1; // cycle back to "no window focused"
    }
    updateFocusFlags();
}

WindowManager::WindowPtr WindowManager::focusedWindow() const {
    if (m_focusIndex >= 0 &&
        m_focusIndex < static_cast<int>(m_windows.size())) {
        return m_windows[static_cast<std::size_t>(m_focusIndex)];
    }
    return nullptr;
}

bool WindowManager::hasFocus() const {
    return m_focusIndex >= 0 && !m_windows.empty();
}

void WindowManager::renderTo(ScreenBuffer& buf) const {
    for (const auto& win : m_windows) {
        win->renderTo(buf);
    }
}

void WindowManager::updateFocusFlags() {
    for (int i = 0; i < static_cast<int>(m_windows.size()); ++i) {
        m_windows[static_cast<std::size_t>(i)]->setFocused(i == m_focusIndex);
    }
}

} // namespace edinz
