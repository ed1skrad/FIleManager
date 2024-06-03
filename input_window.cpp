#include "input_window.h"
#include <ncurses.h>

// Конструктор класса InputWindow, инициализирует ncurses и создает окно ввода
InputWindow::InputWindow(int width, int height) {
    // Инициализируем ncurses
    initscr();
    // Переключаемся в режим raw, чтобы не обрабатывать специальные символы ввода
    raw();
    // Включаем обработку специальных клавиш, таких как стрелки
    keypad(stdscr, TRUE);
    // Отключаем эхо ввода на экран
    noecho();
    // Включаем поддержку цвета
    start_color();

    // Инициализируем пару цветов для выделения сообщения
    init_pair(5, COLOR_RED, COLOR_BLACK);

    // Вычисляем координаты окна ввода, чтобы оно было по центру экрана
    x = (COLS - width) / 2;
    y = (LINES - height) / 2;

    // Создаем окно ввода с заданными размерами и координатами
    win = newwin(height, width, y, x);
    // Рисуем рамку вокруг окна ввода
    box(win, 0, 0);
}

// Деструктор класса InputWindow, очищает окно ввода и завершает ncurses
InputWindow::~InputWindow() {
    // Удаляем окно ввода
    delwin(win);
    // Завершаем ncurses
    endwin();
}

// Метод класса InputWindow, отображает окно ввода с сообщением и возвращает введенную пользователем строку
std::string InputWindow::show(const std::string& message) {
    // Отключаем эхо ввода на экран
    noecho();

    // Очищаем окно ввода и рисуем рамку вокруг него
    wclear(win);
    box(win, 0, 0);

    // Устанавливаем цвет сообщения и выводим его в центре окна ввода
    wattron(win, COLOR_PAIR(5));
    mvwprintw(win, 1, (getmaxx(win) - message.length()) / 2, "%s", message.c_str());
    wattroff(win, COLOR_PAIR(5));

    // Обновляем содержимое окна ввода на экране
    wrefresh(win);

    // Вычисляем координаты и размеры окна ввода для строки ввода пользователя
    int input_box_y = 3;
    int input_box_height = 3;
    int input_box_width = getmaxx(win) - 4;
    int input_box_x = 2;

    // Создаем окно ввода для строки ввода пользователя
    WINDOW* input_box = derwin(win, input_box_height, input_box_width, input_box_y, input_box_x);
    // Рисуем рамку вокруг окна ввода строки ввода пользователя
    box(input_box, 0, 0);
    // Обновляем содержимое окна ввода строки ввода пользователя на экране
    wrefresh(input_box);

    // Инициализируем строку ввода пользователя и максимальную длину ввода
    std::string input;
    std::string::size_type max_input_length = static_cast<std::string::size_type>(input_box_width - 2);

    // Цикл обработки ввода пользователя
    while (true) {
        // Получаем символ ввода пользователя
        int ch = wgetch(input_box);

        // Если символ является символом новой строки, завершаем ввод
        if (ch == '\n') {
            break;
        }
        // Если символ является символом escape, очищаем строку ввода и завершаем ввод
        else if (ch == 27) {
            input.clear();
            break;
        }
        // Если символ является символом backspace, удаляем последний символ из строки ввода
        else if (ch == KEY_BACKSPACE || ch == 127) {
            if (!input.empty()) {
                input.pop_back();
            }
        }
        // Если символ является печатным символом, добавляем его в строку ввода, если длина строки ввода не превышает максимальную длину ввода
        else if (ch >= ' ' && ch <= '~') {
            if (input.length() < max_input_length) {
                input.push_back(ch);
            }
        }

        // Вычисляем начальную позицию строки ввода, чтобы она была по центру окна ввода строки ввода пользователя
        int input_start_pos = (input_box_width - input.length()) / 2;

        // Очищаем строку ввода и выводим ее в центре окна ввода строки ввода пользователя
        mvwhline(input_box, 1, 1, ' ', input_box_width - 2);
        mvwprintw(input_box, 1, input_start_pos, "%s", input.c_str());
        // Обновляем содержимое окна ввода строки ввода пользователя на экране
        wrefresh(input_box);
    }

    // Удаляем окно ввода строки ввода пользователя
    delwin(input_box);

    // Возвращаем введенную пользователем строку
    return input;
}
