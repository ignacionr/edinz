#pragma once

#include "terminal.hpp"

#include <functional>
#include <string>
#include <vector>

namespace edinz {

struct MenuItem {
    std::string label;
    char shortcut; // lowercase letter for Ctrl+<key>, or 0 for none
    std::function<void()> action;
};

class App {
public:
    App();

    // Main entry: runs until quit
    void run();

private:
    void render();
    void handleKey(const KeyEvent& event);
    void moveUp();
    void moveDown();
    void selectCurrent();

    Terminal m_term;
    std::vector<MenuItem> m_menu;
    int m_selected{0};
    bool m_running{true};
    std::string m_status;
};

} // namespace edinz
