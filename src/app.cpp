#include "app.hpp"
#include "version.hpp"

#include <format>
#include <string>

namespace edinz {

App::App() {
    m_menu = {
        {"Open file",  'o', [this] { setStatus("Open file selected"); }},
        {"Save file",  's', [this] { setStatus("Save file selected"); }},
        {"Settings",   'e', [this] { setStatus("Settings selected"); }},
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

    m_term.hideCursor();
    m_term.moveTo(1, 1);

    // Title bar
    std::string title = std::format(" edinz v{} ", edinz::version_full());
    std::string bar(static_cast<std::size_t>(cols), '=');
    auto pad = (cols - static_cast<int>(title.size())) / 2;
    if (pad > 0) {
        bar.replace(static_cast<std::size_t>(pad), title.size(), title);
    }
    m_term.write(bar);
    m_term.write("\r\n\r\n");

    // Menu items
    for (int i = 0; i < static_cast<int>(m_menu.size()); ++i) {
        const auto& item = m_menu[static_cast<std::size_t>(i)];

        std::string shortcut_hint;
        if (item.shortcut) {
            shortcut_hint = std::format("Ctrl+{}", static_cast<char>(item.shortcut - 32));
        }

        if (i == m_selected) {
            m_term.write(std::format("  \x1b[7m > {} \x1b[0m", item.label));
        } else {
            m_term.write(std::format("    {}",  item.label));
        }

        if (!shortcut_hint.empty()) {
            // Right-align the shortcut hint
            auto label_len = item.label.size() + 6; // account for prefix
            auto gap = cols - static_cast<int>(label_len) - static_cast<int>(shortcut_hint.size()) - 2;
            if (gap > 0) {
                m_term.write(std::string(static_cast<std::size_t>(gap), ' '));
            }
            m_term.write(std::format("\x1b[2m{}\x1b[0m", shortcut_hint));
        }

        m_term.write("\x1b[K\r\n");
    }

    // Status line
    m_term.moveTo(rows - 1, 1);
    m_term.write(std::string(static_cast<std::size_t>(cols), ' ')); // clear line
    m_term.moveTo(rows - 1, 1);

    if (!m_status.empty()) {
        m_term.write(std::format("\x1b[33m{}\x1b[0m", m_status));
    }

    // Bottom bar with shortcut reminder
    m_term.moveTo(rows, 1);
    std::string bottom(static_cast<std::size_t>(cols), ' ');
    std::string hint = " Up/Down: navigate | Enter: select | Ctrl+Q: quit ";
    if (static_cast<int>(hint.size()) <= cols) {
        bottom.replace(0, hint.size(), hint);
    }
    m_term.write(std::format("\x1b[7m{}\x1b[0m", bottom));
}

void App::handleKey(const KeyEvent& event) {
    if (auto* key = std::get_if<Key>(&event)) {
        switch (*key) {
            case Key::Up:    moveUp(); break;
            case Key::Down:  moveDown(); break;
            case Key::Enter: selectCurrent(); break;
            case Key::Escape:
                m_status.clear();
                break;
            default: break;
        }
    } else if (auto* ctrl = std::get_if<Ctrl>(&event)) {
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

} // namespace edinz
