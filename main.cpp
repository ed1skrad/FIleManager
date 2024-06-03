#include <iostream>
#include <ncurses.h>
#include <string>
#include <vector>
#include "input_window.h"
#include "help_window.h"
#include "file_panel.h"
#include "file_operations.h"

int main() {
    // Инициализация ncurses
    initscr();
    // Перевод терминала в сырой режим, где каждый символ с клавиатуры передается сразу же
    raw();
    // Обработка специальных символов на клавиатуре
    keypad(stdscr, TRUE);
    // Отключение отображения нажатых символов
    noecho();
    // Инициализация структуры для копирования и вставки файлов
    CopiedFile copied_file_or_directory;
    // Инициализация поддержки цвета в ncurses
    start_color();
    // Инициализация цветовой пары белый на синем
    init_pair(1, COLOR_WHITE, COLOR_BLUE);
    // Установка цвета фона
    bkgd(COLOR_PAIR(1));
    // Обновление экрана для отображения изменений
    refresh();

    // Размеры для левой и правой панели, а также последующая инициализация
    int y = 0;
    int x = 0;
    int h = LINES;
    int w = COLS / 2;

    FilePanel left_panel(y, x, h, w);
    FilePanel right_panel(y, x + w, h, w);

    // Переменная для отслеживания активной панели
    bool active_panel = true;

    // Цикл обработки ввода пользователя
    while (true) {
        // Установка активной панели
        left_panel.set_selected(active_panel);
        right_panel.set_selected(!active_panel);

        // Отрисовка панелей
        left_panel.draw();
        right_panel.draw();

        // Обработка ввода пользователя
        int ch = getch();
        switch (ch) {
            case KEY_UP:
                // Перемещение выделения вверх
                (active_panel ? left_panel : right_panel).move_selection(-1);
                break;
            case KEY_DOWN:
                // Перемещение выделения вниз
                (active_panel ? left_panel : right_panel).move_selection(1);
                break;
            case KEY_LEFT:
                // Переход в родительский каталог
                (active_panel ? left_panel : right_panel).change_directory(-1);
                break;
            case KEY_RIGHT:
                // Переход в выбранный каталог
                if (active_panel) {
                    if (left_panel.is_selected()) {
                        left_panel.change_directory(1);
                    }
                } else {
                    if (right_panel.is_selected()) {
                        right_panel.change_directory(1);
                    }
                }
                break;
            case KEY_DC:
                // Удаление выбранного файла или каталога
                {
                    FilePanel* current_panel = active_panel ? &left_panel : &right_panel;
                    std::string file_path = current_panel->get_current_dir() + "/" + current_panel->get_selected_file();
                    struct stat st;
                    if (stat(file_path.c_str(), &st) == 0) {
                        InputWindow input_window(100, 10);
                        std::string message;
                        if (S_ISDIR(st.st_mode)) {
                            message = "Are you sure you want to delete the directory '" + current_panel->get_selected_file() + "'? (y/n)";
                        } else {
                            message = "Are you sure you want to delete the file '" + current_panel->get_selected_file() + "'? (y/n)";
                        }
                        std::string response = input_window.show(message);
                        if (response == "yes" || response == "y") {
                            std::string command = "rm -r '" + file_path + "'";
                            system(command.c_str());
                            current_panel->update();
                        }
                    }
                }
                break;
            case '\t':
                // Переключение активной панели
                active_panel = !active_panel;
                break;
            case KEY_BACKSPACE:
                // Переход в родительский каталог
                if (active_panel) {
                    left_panel.change_directory(-1);
                } else {
                    right_panel.change_directory(-1);
                }
                break;
            case 10:
                // Переход в выбранный каталог
                if (active_panel) {
                    left_panel.change_directory(1);
                } else {
                    right_panel.change_directory(1);
                }
                break;
            case 't':
                // Создание новой вкладки
                (active_panel ? left_panel : right_panel).create_tab();
                break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                // Переключение на выбранную вкладку
                (active_panel ? left_panel : right_panel).switch_to_tab(ch - '0');
                break;
            case 'T':
                // Отображение списка вкладок
                (active_panel ? left_panel : right_panel).show_tabs();
                break;
            case KEY_F(12):
                // Удаление выбранной вкладки
                {
                    InputWindow input_window(100, 10);
                    std::string message = "Enter the tab number to delete (0-9): ";
                    std::string response = input_window.show(message);

                    int tab_index = std::stoi(response);

                    if (tab_index >= 0 && tab_index < 9) {
                        std::string tab_path = active_panel ? left_panel.get_tab_by_index(tab_index) : right_panel.get_tab_by_index(tab_index);
                        if (!tab_path.empty()) {
                            active_panel ? left_panel.delete_tab(tab_index) : right_panel.delete_tab(tab_index);
                            active_panel ? left_panel.set_current_tab_index(0) : right_panel.set_current_tab_index(0);
                            active_panel ? left_panel.update() : right_panel.update();
                        }
                    }

                    int y, x;
                    getyx(stdscr, y, x);
                    mvprintw(y, 0, " %*s", COLS, "");
                    refresh();
                }
                break;
            case KEY_F(2):
                // Переименование выбранного файла или каталога
                (active_panel ? left_panel : right_panel).rename_file_or_directory();
                break;
            case 'n':
                // Создание нового файла
                {
                    FilePanel* current_panel = active_panel ? &left_panel : &right_panel;
                    InputWindow input_window(120, 8);
                    std::string message = "Enter new file name: ";
                    std::string response = input_window.show(message);
                    std::string file_path = current_panel->get_current_dir() + "/" + response;
                    create_file(file_path);
                    current_panel->update();
                }
                break;
            case 'm':
                // Создание нового каталога
                {
                    FilePanel* current_panel = active_panel ? &left_panel : &right_panel;
                    InputWindow input_window(120, 8);
                    std::string message = "Enter new directory name: ";
                    std::string response = input_window.show(message);
                    std::string dir_path = current_panel->get_current_dir() + "/" + response;
                    create_directory(dir_path);
                    current_panel->update();
                }
                break;
            case 'c':
                // Копирование выбранного файла или каталога
                (active_panel ? left_panel : right_panel).copy_file_or_directory();
                break;
            case 'v':
                // Вставка скопированного файла или каталога
                (active_panel ? left_panel : right_panel).paste_file_or_directory();
                break;
            case 'o':
                // Открытие выбранного файла
                (active_panel ? left_panel : right_panel).open_file();
                break;
            case 'h':
                // Отображение окна помощи
                {
                    HelpWindow help_window(35, 70);
                    help_window.show();
                }
                break;
            case 'i':
                // Отображение информации о выбранном файле
                (active_panel ? left_panel : right_panel).show_file_info();
                break;
            case 'q':
                // Выход из программы
                endwin();
                return 0;
            case KEY_F(5):
                // Сброс размеров и положений панелей
                left_panel.set_size(LINES, COLS / 2);
                right_panel.set_size(LINES, COLS / 2);

                left_panel.set_position(0, 0);
                right_panel.set_position(0, COLS / 2);

                clear();
                refresh();
                right_panel.draw();
                left_panel.draw();
                break;
            default:
                break;
        }

        // Скрытие курсора
        curs_set(0);
    }

    // Завершение ncurses
    endwin();
    return 0;
}
