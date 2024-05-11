#ifndef HELP_WINDOW_H
#define HELP_WINDOW_H

#include <ncurses.h>

class HelpWindow {
public:
    HelpWindow(int height, int width);
    ~HelpWindow();

    void show();

private:
    WINDOW* win;
    int x, y;
};

#endif // HELP_WINDOW_H
