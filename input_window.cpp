#include "input_window.h"

InputWindow::InputWindow(int width, int height) {
    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();

    x = (COLS - width) / 2;
    y = (LINES - height) / 2;

    win = newwin(height, width, y, x);
    box(win, 0, 0);
}

InputWindow::~InputWindow() {
    delwin(win);
    endwin();
}

std::string InputWindow::show(const std::string& message) {
    noecho();

    wclear(win);
    box(win, 0, 0);
    mvwprintw(win, 1, (x - message.length()) / 2, "%s", message.c_str());
    wmove(win, 2, x / 2 - message.length() / 2 + message.length());
    wrefresh(win);

    std::string input;
    while (true) {
        int ch = wgetch(win);
        if (ch == '\n' || ch == 27) { // 27 - это ASCII-код клавиши Esc
            break;
        } else if (ch == KEY_BACKSPACE || ch == 127) { // 127 - это ASCII-код клавиши Delete
            if (!input.empty()) {
                input.pop_back();
                wmove(win, 2, x / 2 - message.length() / 2 + message.length() + input.length());
                wprintw(win, " \b");
                wrefresh(win);
            }
        } else if (ch >= ' ' && ch <= '~') {
            input.push_back(ch);
            wprintw(win, "%c", ch);
            wrefresh(win);
        }
    }

    return input;
}




