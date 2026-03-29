#pragma once

#include "terminal.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace edinz {

// A single cell in the screen buffer
struct Cell {
    char32_t ch{U' '};
    uint8_t attr{0};

    static constexpr uint8_t Bold    = 0x01;
    static constexpr uint8_t Dim     = 0x02;
    static constexpr uint8_t Reverse = 0x04;
};

// 2D screen buffer for composited rendering (0-indexed)
class ScreenBuffer {
public:
    ScreenBuffer(int rows, int cols);

    int rows() const { return m_rows; }
    int cols() const { return m_cols; }

    Cell& at(int row, int col);
    const Cell& at(int row, int col) const;

    void writeText(int row, int col, std::string_view text, uint8_t attr = 0);
    void drawBox(int row, int col, int height, int width,
                 bool doubleLine = false, uint8_t attr = 0);
    void fillRect(int row, int col, int height, int width,
                  char32_t ch = U' ', uint8_t attr = 0);

    // Flush entire buffer to terminal in one write
    void renderTo(Terminal& term) const;

private:
    int m_rows;
    int m_cols;
    std::vector<std::vector<Cell>> m_cells;
};

// A rectangular window rendered into a ScreenBuffer
class Window {
public:
    Window(int row, int col, int height, int width, std::string title);

    int row() const { return m_row; }
    int col() const { return m_col; }
    int height() const { return m_height; }
    int width() const { return m_width; }

    void move(int row, int col);
    void resize(int height, int width);

    const std::string& title() const { return m_title; }
    void setTitle(const std::string& title) { m_title = title; }

    bool focused() const { return m_focused; }
    void setFocused(bool f) { m_focused = f; }

    bool visible() const { return m_visible; }
    void setVisible(bool v) { m_visible = v; }

    void setContent(std::vector<std::string> lines);
    void addLine(const std::string& line);
    void clearContent();

    void scrollUp();
    void scrollDown();

    void renderTo(ScreenBuffer& buf) const;

private:
    int m_row;
    int m_col;
    int m_height;
    int m_width;
    std::string m_title;
    std::vector<std::string> m_content;
    int m_scrollOffset{0};
    bool m_focused{false};
    bool m_visible{true};
};

// Manages a z-ordered stack of overlapping windows
class WindowManager {
public:
    using WindowPtr = std::shared_ptr<Window>;

    WindowPtr createWindow(int row, int col, int height, int width,
                           const std::string& title);
    void closeWindow(const WindowPtr& win);
    void closeFocused();

    // Cycle focus: ... -> window0 -> window1 -> ... -> windowN -> (none) -> window0 ...
    void focusNext();

    WindowPtr focusedWindow() const;
    bool hasFocus() const;

    void renderTo(ScreenBuffer& buf) const;

    bool empty() const { return m_windows.empty(); }

private:
    std::vector<WindowPtr> m_windows; // last element drawn on top
    int m_focusIndex{-1};

    void updateFocusFlags();
};

} // namespace edinz
