#include "help_window.h"
#include <ncurses.h>

HelpWindow::HelpWindow(int height, int width) {
    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();
    start_color();

    init_pair(2, COLOR_YELLOW, COLOR_BLACK);
    init_pair(3, COLOR_WHITE, COLOR_BLACK);

    x = (COLS - width) / 2;
    y = (LINES - height) / 2;

    win = newwin(height, width, y, x);
    box(win, 0, 0);
}

HelpWindow::~HelpWindow() {
    delwin(win);
    endwin();
}

void HelpWindow::show() {
    werase(win);
    box(win, 0, 0);

    // Display help text with sections
    wattron(win, COLOR_PAIR(3));
    mvwprintw(win, 2, 2, "Use the arrow keys to navigate through the files and directories.");
    mvwhline(win, 3, 1, '-', getmaxx(win) - 2);

    mvwprintw(win, 4, 2, "Press Tab to switch between panels.");
    mvwprintw(win, 5, 2, "Press Enter to open a file or enter a directory.");
    mvwprintw(win, 6, 2, "Press Backspace to go back to the parent directory.");
    mvwhline(win, 7, 1, '-', getmaxx(win) - 2);

    mvwprintw(win, 8, 2, "Press 't' to create a new tab.");
    mvwprintw(win, 9, 2, "Press '0'-'9' to switch to a specific tab.");
    mvwprintw(win, 10, 2, "Press 'T' to show all tabs.");
    mvwhline(win, 11, 1, '-', getmaxx(win) - 2);

    mvwprintw(win, 12, 2, "Press 'c' to copy a file or directory.");
    mvwprintw(win, 13, 2, "Press 'v' to paste a file or directory.");
    mvwprintw(win, 14, 2, "Press 'o' to open a file.");
    mvwprintw(win, 15, 2, "Press 'i' to get info about file.");
    mvwhline(win, 16, 1, '-', getmaxx(win) - 2);

    mvwprintw(win, 17, 2, "Press 'q' to quit the program.");
    wattroff(win, COLOR_PAIR(3));

    wattron(win, COLOR_PAIR(2));
    mvwprintw(win, 1, 1, "Help:");
    wattroff(win, COLOR_PAIR(2));

    wrefresh(win);
    getch();
}
