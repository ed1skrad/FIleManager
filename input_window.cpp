#include "input_window.h"
#include <ncurses.h>

InputWindow::InputWindow(int width, int height) {
    // Initialize ncurses and create the input window
    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();
    start_color();

    init_pair(5, COLOR_RED, COLOR_BLACK);

    x = (COLS - width) / 2;
    y = (LINES - height) / 2;

    win = newwin(height, width, y, x);
    box(win, 0, 0);
}

InputWindow::~InputWindow() {
    // Clean up the input window and end ncurses
    delwin(win);
    endwin();
}

std::string InputWindow::show(const std::string& message) {
    // Clear the input window and display the message
    noecho();

    wclear(win);
    box(win, 0, 0);

    wattron(win, COLOR_PAIR(5));
    mvwprintw(win, 1, (getmaxx(win) - message.length()) / 2, "%s", message.c_str());
    wattroff(win, COLOR_PAIR(5));

    wrefresh(win);

    // Get user input and return it as a string
    std::string input;
    std::string::size_type max_input_length = static_cast<std::string::size_type>(getmaxx(win) - 4); // 4 is an arbitrary padding value to avoid overflow
    while (true) {
        int ch = wgetch(win);
        if (ch == '\n' || ch == 27) {
            break;
        } else if (ch == KEY_BACKSPACE || ch == 127) {
            if (!input.empty()) {
                input.pop_back();
            }
        } else if (ch >= ' ' && ch <= '~') {
            if (input.length() < max_input_length) {
                input.push_back(ch);
            }
        }

        // Recalculate the starting position for the input to keep it centered
        int input_start_pos = (getmaxx(win) - input.length()) / 2;

        // Clear the input line and reprint the input
        mvwhline(win, 2, 1, ' ', getmaxx(win) - 2); // Clear the input line
        mvwprintw(win, 2, input_start_pos, "%s", input.c_str());
        wrefresh(win);
    }

    return input;
}
