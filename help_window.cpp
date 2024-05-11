#include "help_window.h"

HelpWindow::HelpWindow(int height, int width) {
    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();

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

    mvwprintw(win, 1, 1, "Help:");
    mvwprintw(win, 2, 2, "Use the arrow keys to navigate through the files and directories.");
    mvwprintw(win, 3, 2, "Press Tab to switch between panels.");
    mvwprintw(win, 4, 2, "Press Enter to open a file or enter a directory.");
    mvwprintw(win, 5, 2, "Press Backspace to go back to the parent directory.");
    mvwprintw(win, 6, 2, "Press 't' to create a new tab.");
    mvwprintw(win, 7, 2, "Press '0'-'9' to switch to a specific tab.");
    mvwprintw(win, 8, 2, "Press 'T' to show all tabs.");
    mvwprintw(win, 9, 2, "Press 'c' to copy a file or directory.");
    mvwprintw(win, 10, 2, "Press 'v' to paste a file or directory.");
    mvwprintw(win, 11, 2, "Press 'o' to open a file.");
    mvwprintw(win, 12, 2, "Press 'q' to quit the program.");

    wrefresh(win);
    getch();
}
