#ifndef INPUT_WINDOW_H
#define INPUT_WINDOW_H

#include <ncurses.h>
#include <string>

class InputWindow {
public:
    InputWindow(int width, int height);
    ~InputWindow();

    std::string show(const std::string& message);

private:
    WINDOW* win;
    int x, y;
};

#endif
