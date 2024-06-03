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

    // Define the position and size of the input box
    int input_box_y = 3;
    int input_box_height = 3;
    int input_box_width = getmaxx(win) - 4; // Leave some padding for the box
    int input_box_x = 2;

    // Create a sub-window for the input box
    WINDOW* input_box = derwin(win, input_box_height, input_box_width, input_box_y, input_box_x);
    box(input_box, 0, 0);
    wrefresh(input_box);

    // Get user input and return it as a string
    std::string input;
    std::string::size_type max_input_length = static_cast<std::string::size_type>(input_box_width - 2); // Adjust for box borders
    while (true) {
        int ch = wgetch(input_box);
        if (ch == '\n') {
            break;
        } else if (ch == 27) {
            input.clear();
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
        int input_start_pos = (input_box_width - input.length()) / 2;

        // Clear the input line and reprint the input
        mvwhline(input_box, 1, 1, ' ', input_box_width - 2); // Clear the input line
        mvwprintw(input_box, 1, input_start_pos, "%s", input.c_str());
        wrefresh(input_box);
    }

    // Cleanup the input box window
    delwin(input_box);

    return input;
}
