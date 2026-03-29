#include "app.hpp"
#include "version.hpp"

#include <format>
#include <string>

namespace edinz {

App::App() {
    m_menu = {
        {"Open file",  'o', [this] { openFileWindow(); }},
        {"Save file",  's', [this] { setStatus("Save file selected"); }},
        {"Settings",   'e', [this] { openSettingsWindow(); }},
        {"Quit",       'q', [this] { m_running = false; }},
    };
}

void App::run() {
    m_term.clear();
    m_term.hideCursor();

    while (m_running) {
        render();

        auto event = m_term.readKey();
        if (event) {
            handleKey(*event);
        }
    }

    m_term.showCursor();
    m_term.clear();
    m_term.moveTo(1, 1);
    m_term.write("Goodbye.\r\n");
}

void App::render() {
    auto [rows, cols] = m_term.size();
    ScreenBuffer buf(rows, cols);

    // === Title bar (row 0) ===
    std::string title = std::format(" edinz v{} ", edinz::version_full());
    std::string bar(static_cast<std::size_t>(cols), '=');
    auto pad = (cols - static_cast<int>(title.size())) / 2;
    if (pad > 0) {
        bar.replace(static_cast<std::size_t>(pad), title.size(), title);
    }
    buf.writeText(0, 0, bar);

    // === Menu items (starting at row 2) ===
    bool menuActive = !m_wm.hasFocus();

    for (int i = 0; i < static_cast<int>(m_menu.size()); ++i) {
        const auto& item = m_menu[static_cast<std::size_t>(i)];

        std::string shortcut_hint;
        if (item.shortcut) {
            shortcut_hint = std::format("Ctrl+{}", static_cast<char>(item.shortcut - 32));
        }

        bool selected = (i == m_selected);
        if (selected && menuActive) {
            buf.writeText(2 + i, 2, std::format(" > {} ", item.label), Cell::Reverse);
        } else if (selected) {
            buf.writeText(2 + i, 2, std::format(" > {} ", item.label), Cell::Dim);
        } else {
            buf.writeText(2 + i, 4, item.label);
        }

        if (!shortcut_hint.empty()) {
            int hintCol = cols - static_cast<int>(shortcut_hint.size()) - 2;
            if (hintCol > 0) {
                buf.writeText(2 + i, hintCol, shortcut_hint, Cell::Dim);
            }
        }
    }

    // === Status line ===
    if (!m_status.empty()) {
        buf.writeText(rows - 2, 1, m_status);
    }

    // === Bottom bar ===
    std::string hint =
        " Up/Down: navigate | Enter: select | Tab: cycle windows "
        "| Ctrl+W: close | Ctrl+Q: quit ";
    std::string bottom(static_cast<std::size_t>(cols), ' ');
    if (static_cast<int>(hint.size()) <= cols) {
        bottom.replace(0, hint.size(), hint);
    }
    buf.writeText(rows - 1, 0, bottom, Cell::Reverse);

    // === Overlay windows ===
    m_wm.renderTo(buf);

    // === Flush ===
    m_term.hideCursor();
    buf.renderTo(m_term);
}

void App::handleKey(const KeyEvent& event) {
    if (auto* key = std::get_if<Key>(&event)) {
        if (m_wm.hasFocus()) {
            switch (*key) {
                case Key::Up:
                    if (auto w = m_wm.focusedWindow()) w->scrollUp();
                    return;
                case Key::Down:
                    if (auto w = m_wm.focusedWindow()) w->scrollDown();
                    return;
                case Key::Escape:
                    m_status.clear();
                    return;
                case Key::Tab:
                    m_wm.focusNext();
                    return;
                default:
                    return;
            }
        }

        switch (*key) {
            case Key::Up:    moveUp(); break;
            case Key::Down:  moveDown(); break;
            case Key::Enter: selectCurrent(); break;
            case Key::Escape:
                m_status.clear();
                break;
            case Key::Tab:
                if (!m_wm.empty()) m_wm.focusNext();
                break;
            default: break;
        }
    } else if (auto* ctrl = std::get_if<Ctrl>(&event)) {
        if (ctrl->ch == 'w') {
            m_wm.closeFocused();
            return;
        }
        // Match against menu shortcuts
        for (const auto& item : m_menu) {
            if (item.shortcut == ctrl->ch) {
                item.action();
                return;
            }
        }
    }
}

void App::moveUp() {
    if (m_selected > 0) {
        --m_selected;
    }
}

void App::moveDown() {
    if (m_selected < static_cast<int>(m_menu.size()) - 1) {
        ++m_selected;
    }
}

void App::selectCurrent() {
    auto idx = static_cast<std::size_t>(m_selected);
    if (idx < m_menu.size() && m_menu[idx].action) {
        m_menu[idx].action();
    }
}

void App::openFileWindow() {
    int offset = (m_windowCount % 5) * 2;
    ++m_windowCount;
    auto win = m_wm.createWindow(3 + offset, 5 + offset, 14, 40, "Open File");
    win->setContent({
        "Files:",
        "",
        "  CMakeLists.txt",
        "  flake.nix",
        "  src/",
        "    main.cpp",
        "    app.cpp",
        "    app.hpp",
        "    terminal.cpp",
        "    terminal.hpp",
        "    window.cpp",
        "    window.hpp",
    });
    setStatus("Opened file browser window");
}

void App::openSettingsWindow() {
    int offset = (m_windowCount % 5) * 2;
    ++m_windowCount;
    auto win = m_wm.createWindow(4 + offset, 20 + offset, 10, 35, "Settings");
    win->setContent({
        "Configuration:",
        "",
        "  Theme:        Dark",
        "  Font size:    14",
        "  Tab size:     4",
        "  Line numbers: On",
        "  Word wrap:    Off",
    });
    setStatus("Opened settings window");
}

} // namespace edinz
