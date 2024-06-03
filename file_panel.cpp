#include "file_panel.h"
#include <algorithm>
#include <cstring>
#include <dirent.h>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ncurses.h>
#include <fcntl.h>
#include "input_window.h"
#include <ctime>

#define MAX_TABS 10

CopiedFile copied_file_or_directory;

int FilePanel::get_selected_file_index() const {
    return selected_file;
}

int FilePanel::get_files_size() const {
    return files.size();
}

std::string FilePanel::get_current_dir() const {
    return current_dir;
}

std::string FilePanel::get_selected_file() const {
    if (selected_file >= 0 && selected_file < static_cast<int>(files.size())) {
        return files[selected_file];
    }
    return "";
}

FilePanel::FilePanel(int start_y, int start_x, int height, int width)
        : y(start_y), x(start_x), h(height), w(width), selected(false), scroll_position(0), max_scroll_position(0) {
    win = newwin(h, w, y, x);
    selected_file = 0;
    getcwd(current_dir_cstr, PATH_MAX);
    current_dir = current_dir_cstr;
    current_tab_index = 0;

    init_pair(1, COLOR_WHITE, COLOR_BLACK); // Цвет для файлов
    init_pair(2, COLOR_CYAN, COLOR_BLACK); // Цвет для директорий
    init_pair(3, COLOR_YELLOW, COLOR_BLACK); // Цвет для символических ссылок
    init_pair(4, COLOR_WHITE, COLOR_BLACK); // Новая цветовая пара с более темным фоном
    list_directory();
}

FilePanel::~FilePanel() {
    delwin(win);
}

void FilePanel::draw() {
     // Очищаем содержимое окна
    werase(win);

    // Устанавливаем цвет фона окна
    wbkgd(win, COLOR_PAIR(4));

    // Рисуем рамку вокруг окна
    box(win, 0, 0);

    // Выводим заголовок "File Manager" в центре верхней строки окна
    mvwprintw(win, 0, (w - 13) / 2, "File Manager");

    // Выводим текущий путь в первой строке окна
    std::string current_dir_display = current_dir;
    if (current_dir_display.back() == '/' && current_dir_display.size() > 1) {
        current_dir_display.pop_back();
    }
    mvwprintw(win, 1, 1, "Current path: %s", current_dir_display.c_str());

    // Выводим заголовки колонок "Filename", "Size" и "Last Modified" во второй строке окна
    mvwprintw(win, 2, 1, "Filename");
    mvwprintw(win, 2, w / 3, "Size");
    mvwprintw(win, 2, 2 * w / 3, "Last Modified");

    // Рисуем горизонтальную линию после заголовков
    mvwhline(win, 3, 1, ACS_HLINE, w - 2);

    // Если в текущем каталоге нет файлов, выводим сообщение "No files in this directory" в центре окна
    if (files.empty()) {
        mvwprintw(win, h / 2, (w - 20) / 2, "No files in this directory");
    } else {
        // Определяем индексы первого и последнего файлов, которые будут отображены в текущем окне
        int start_index = scroll_position;
        int end_index = std::min(scroll_position + h - 5, static_cast<int>(files.size()));

        // Отображаем файлы в соответствующих колонках
        for (int i = start_index; i < end_index; ++i) {
            // Если текущий файл является выбранным, устанавливаем атрибут A_REVERSE для выделения
            if (i == selected_file && selected)
                wattron(win, A_REVERSE);

            // Получаем информацию о файле
            struct stat st;
            std::string file_path = current_dir + "/" + files[i];
            if (stat(file_path.c_str(), &st) == 0) {
                // Получаем время последней модификации файла
                time_t last_modified = st.st_mtime;
                tm* time_info = localtime(&last_modified);

                // Форматируем время в строку
                char time_str[20];
                strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M", time_info);

                // Получаем размер файла
                std::string size = std::to_string(st.st_size);

                // Устанавливаем цвет текста в зависимости от типа файла
                if (S_ISDIR(st.st_mode)) {
                    wattron(win, COLOR_PAIR(2)); // Устанавливаем цвет для директорий
                } else if (S_ISLNK(st.st_mode)) {
                    wattron(win, COLOR_PAIR(3)); // Устанавливаем цвет для символических ссылок
                } else {
                    wattron(win, COLOR_PAIR(1)); // Устанавливаем цвет для файлов
                }

                // Выводим имя файла, размер и время последней модификации в соответствующих колонках
                mvwprintw(win, i - scroll_position + 4, 1, "%s", files[i].c_str());
                wattroff(win, COLOR_PAIR(2) | COLOR_PAIR(3)); // Сбрасываем цвет для директорий и символических ссылок
                mvwprintw(win, i - scroll_position + 4, w / 3, "%s", size.c_str());
                mvwprintw(win, i - scroll_position + 4, 2 * w / 3, "%s", time_str);

                // Сбрасываем цвет фона
                wattroff(win, COLOR_PAIR(1));
            } else {
                // Если не удалось получить информацию о файле, выводим только имя файла
                mvwprintw(win, i - scroll_position + 4, 1, "%s", files[i].c_str());
            }

            // Сбрасываем атрибут A_REVERSE
            wattroff(win, A_REVERSE);
        }
    }
}

void FilePanel::update() {
    // Обновляем список файлов в текущем каталоге
    list_directory();

    // Сбрасываем позицию прокрутки в начало списка
    scroll_position = 0;

    // Вычисляем максимальную позицию прокрутки в зависимости от количества файлов и размера окна
    max_scroll_position = std::max(0, static_cast<int>(files.size()) - h + 5);

    // Перерисовываем содержимое окна
    draw();
}

void FilePanel::move_selection(int dir) {
    // Изменяем индекс выделенного элемента на dir
    selected_file += dir;

    // Если индекс выделенного элемента выходит за пределы списка файлов, переходим к первому или последнему элементу
    if (selected_file < 0)
        selected_file = files.size() - 1;
    else if (selected_file >= static_cast<int>(files.size()))
        selected_file = 0;

    // Если выделенный элемент выходит за пределы видимой области, изменяем позицию прокрутки
    if (selected_file < scroll_position)
        scroll_position = selected_file;
    else if (selected_file >= scroll_position + h - 5)
        scroll_position = selected_file - h + 6;

    // Перерисовываем содержимое окна
    draw();
}


void FilePanel::change_directory(int dir) {
    // Если список файлов пуст, выходим из функции
    if (files.empty()) {
        return;
    }

    // Объявляем переменную для хранения нового пути
    std::string new_dir;

    // Если dir равно -1, переходим в родительский каталог
    if (dir == -1) {
        // Если текущий каталог уже корневой, выходим из функции
        if (current_dir == "/")
            return;
        // Иначе находим путь родительского каталога
        new_dir = current_dir.substr(0, current_dir.find_last_of('/'));
        // Если путь родительского каталога пуст, устанавливаем его в "/"
        if (new_dir.empty())
            new_dir = "/";
    }
    // Если dir равно 1, переходим в выбранный каталог
    else if (dir == 1) {
        // Если выбранный элемент выходит за пределы списка файлов или является "." или "..", выходим из функции
        if (selected_file >= static_cast<int>(files.size()) || files[selected_file] == ".." || files[selected_file] == ".")
            return;
        // Иначе формируем путь к выбранному каталогу
        new_dir = current_dir;
        if (new_dir != "/") {
            new_dir += "/";
        }
        new_dir += files[selected_file];
    }

    // Проверяем, существует ли каталог по новому пути и является ли он директорией
    struct stat st;
    if (stat(new_dir.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
        // Если да, обновляем текущий каталог и перерисовываем содержимое окна
        current_dir = new_dir;
        strcpy(current_dir_cstr, current_dir.c_str());
        update();
    } else {
        // Иначе выводим сообщение об ошибке
        printw("Error: cannot change directory to %s\n", new_dir.c_str());
        refresh();
    }

    // Выводим отладочную информацию
    printw("Dir: %d, Selected file: %d, Current dir: %s, New dir: %s\n", dir, selected_file, current_dir.c_str(), new_dir.c_str());
    refresh();
}

void FilePanel::list_directory() {
    // Очищаем список файлов
    files.clear();

    // Открываем текущий каталог
    DIR* dir = opendir(current_dir_cstr);
    // Если не удалось открыть каталог, выходим из функции
    if (!dir)
        return;

    // Читаем содержимое каталога по одному элементу за раз
    dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        // Если элемент является "..", добавляем его в начало списка
        if (strcmp(entry->d_name, "..") == 0) {
            files.insert(files.begin(), entry->d_name);
        }
        // Если элемент не является ".", добавляем его в конец списка
        else if (strcmp(entry->d_name, ".") != 0) {
            files.push_back(entry->d_name);
        }
    }

    // Закрываем каталог
    closedir(dir);
}


bool FilePanel::is_selected() const {
    return selected;
}

void FilePanel::set_selected(bool selected) {
    this->selected = selected;
}

void FilePanel::create_tab() {
    // Итерируемся по всем вкладкам
    for (int i = 0; i < MAX_TABS; ++i) {
        // Если вкладка пуста, добавляем на нее текущий каталог и устанавливаем ее как текущую
        if (tabs[i].empty()) {
            tabs[i] = current_dir;
            current_tab_index = i;
            break;
        }
        // Если вкладка уже содержит текущий каталог, устанавливаем ее как текущую и выходим из функции
        else if (tabs[i] == current_dir) {
            current_tab_index = i;
            return;
        }
    }
}

void FilePanel::switch_to_tab(int tab_index) {
    // Если индекс вкладки находится в допустимых пределах и вкладка не пуста,
    // устанавливаем текущий каталог на каталог вкладки и обновляем содержимое окна
    if (tab_index >= 0 && tab_index < 10 && !tabs[tab_index].empty()) {
        current_dir = tabs[tab_index];
        strcpy(current_dir_cstr, current_dir.c_str());
        update();
    }
}

void FilePanel::show_tabs() {
    int tab_win_y = LINES - 15;
    int tab_win_x = 0;
    int tab_win_h = 15;
    int tab_win_w = COLS / 3;

    WINDOW* tab_win = newwin(tab_win_h, tab_win_w, tab_win_y, tab_win_x);
    werase(tab_win);
    box(tab_win, 0, 0);

    mvwprintw(tab_win, 0, (tab_win_w - 4) / 2, "Tabs"); // Добавляем надпись "Tabs" вверху окна

    bool has_tabs = false; // Флаг, указывающий наличие вкладок
    for (int i = 0; i < 10; ++i) {
        if (!tabs[i].empty()) {
            mvwprintw(tab_win, i + 1, 1, "%d: %s", i, tabs[i].c_str());
            has_tabs = true; // Устанавливаем флаг в true, если вкладка найдена
        }
    }

    if (!has_tabs) {
        mvwprintw(tab_win, tab_win_h / 2, (tab_win_w - 14) / 2, "Here is no tabs"); // Выводим сообщение "Here is no tabs", если вкладок нет
    }

    wrefresh(tab_win);
    getch();
    werase(tab_win);
    wrefresh(tab_win);
    delwin(tab_win);
}

void FilePanel::delete_tab(int index) {
    // Если индекс вкладки выходит за пределы допустимых значений, выходим из функции
    if (index < 0 || index >= MAX_TABS) {
        return;
    }

    // Если вкладка пуста, выходим из функции
    if (tabs[index].empty()) {
        return;
    }

    // Удаляем вкладку, устанавливая ее значение в пустую строку
    tabs[index] = "";

    // Если удаленная вкладка была текущей, устанавливаем текущей первую вкладку и переходим в ее каталог
    if (index == current_tab_index) {
        current_tab_index = 0;
        change_directory(0);
    }
    // Если удаленная вкладка была не текущей и находилась перед текущей, сдвигаем текущую вкладку на одну позицию влево
    else if (index < current_tab_index) {
        current_tab_index--;
    }
}


std::string FilePanel::get_tab_by_index(int index) const {
    if (index >= 0 && index < 9) {
        return tabs[index];
    }
    return "";
}

void FilePanel::set_current_tab_index(int index) {
    if (index >= 0 && index < 9) {
        current_tab_index = index;
        change_directory(0);
    }
}

int FilePanel::get_current_tab_index() const {
    return current_tab_index;
}

void FilePanel::rename_file_or_directory() {
    // Получаем индекс выбранного файла или каталога
    int selected_file_index = get_selected_file_index();
    // Если файл или каталог не выбран, выводим сообщение об ошибке и выходим из функции
    if (selected_file_index == -1) {
        printw("No file or directory selected");
        refresh();
        return;
    }

    // Получаем старое имя файла или каталога
    std::string old_name = get_selected_file();
    std::string new_name;

    // Создаем окно для ввода нового имени
    InputWindow input_window(120, 8);
    std::string message = "Enter new name for " + old_name + ":";

    // Пока пользователь не введет корректное новое имя, запрашиваем его снова
    while (true) {
        new_name = input_window.show(message);

        // Проверяем, что новое имя не пустое и не совпадает с именем другого файла или каталога в текущем каталоге
        if (new_name.empty() || std::find_if(files.begin(), files.end(),
                                             [&new_name](const std::string& file) {
                                                 return file == new_name;
                                             }) != files.end()) {
            message = "Invalid name. Enter new name: ";
            continue;
        }

        break;
    }

    // Формируем старый и новый пути к файлу или каталогу
    std::string old_path = current_dir + "/" + old_name;
    std::string new_path = current_dir + "/" + new_name;

    // Пытаемся переименовать файл или каталог
    if (rename(old_path.c_str(), new_path.c_str()) == 0) {
        // Если переименование успешно, обновляем содержимое окна
        clearok(stdscr, TRUE);
        update();
    } else {
        // Если переименование не удалось, выводим сообщение об ошибке и запрашиваем новое имя снова
        message = "Failed to rename file or directory. Enter new name: ";
    }

    // Скрываем курсор
    curs_set(0);
}

void FilePanel::copy_file_or_directory() {
    // Если выбран файл или каталог, копируем его путь и указатель на текущий объект FilePanel в глобальную переменную copied_file_or_directory
    if (selected_file >= 0 && selected_file < static_cast<int>(files.size())) {
        copied_file_or_directory.file_path = current_dir + "/" + files[selected_file];
        copied_file_or_directory.source_panel = this;
        printw("Copied: %s\n", copied_file_or_directory.file_path.c_str());
        refresh();
    }
    // Если файл или каталог не выбран, выводим сообщение об ошибке
    else {
        printw("No file or directory selected to copy.\n");
        refresh();
    }
}

void FilePanel::paste_file_or_directory() {
    // Если скопированный файл или каталог не пуст, выполняем операцию вставки
    if (!copied_file_or_directory.file_path.empty()) {
        // Формируем путь к целевому каталогу
        std::string destination = current_dir;
        // Получаем информацию о скопированном файле или каталоге
        struct stat st;
        if (stat(copied_file_or_directory.file_path.c_str(), &st) == 0) {
            // Формируем путь к целевому файлу или каталогу
            std::string target_file = destination + "/" + copied_file_or_directory.file_path.substr(copied_file_or_directory.file_path.find_last_of('/') + 1);
            // Проверяем, существует ли целевой файл или каталог
            if (stat(target_file.c_str(), &st) == 0) {
                // Если существует, запрашиваем у пользователя разрешение на перезапись
                InputWindow input_window(120, 8);
                std::string message = "File already exists. Overwrite? (y/n)";
                std::string response = input_window.show(message);
                // Если пользователь разрешил перезапись, выполняем операцию вставки
                if (response == "y" || response == "Y") {
                    // Если скопированный элемент является каталогом, копируем его рекурсивно
                    if (S_ISDIR(st.st_mode)) {
                        std::string command = "cp -r '" + copied_file_or_directory.file_path + "' '" + destination + "'";
                        printw("Pasting directory: %s\n", command.c_str());
                        refresh();
                        system(command.c_str());
                    }
                    // Если скопированный элемент является файлом, копируем его
                    else {
                        std::string command = "cp '" + copied_file_or_directory.file_path + "' '" + destination + "'";
                        printw("Pasting file: %s\n", command.c_str());
                        refresh();
                        system(command.c_str());
                    }
                    // Обновляем содержимое окна
                    update();
                }
            }
            // Если целевой файл или каталог не существует, выполняем операцию вставки
            else {
                // Если скопированный элемент является каталогом, копируем его рекурсивно
                if (S_ISDIR(st.st_mode)) {
                    std::string command = "cp -r '" + copied_file_or_directory.file_path + "' '" + destination + "'";
                    printw("Pasting directory: %s\n", command.c_str());
                    refresh();
                    system(command.c_str());
                }
                // Если скопированный элемент является файлом, копируем его
                else {
                    std::string command = "cp '" + copied_file_or_directory.file_path + "' '" + destination + "'";
                    printw("Pasting file: %s\n", command.c_str());
                    refresh();
                    system(command.c_str());
                }
                // Обновляем содержимое окна
                update();
            }
        }
        // Если не удалось получить информацию о скопированном файле или каталоге, выводим сообщение об ошибке
        else {
            printw("Error: Unable to stat the copied file or directory.\n");
            refresh();
        }
    }
    // Если скопированный файл или каталог пуст, выводим сообщение об ошибке
    else {
        printw("Error: Nothing has been copied to paste.\n");
        refresh();
    }
}

void FilePanel::open_file() {
    // Если выбран файл, выполняем операцию открытия
    if (selected_file >= 0 && selected_file < static_cast<int>(files.size())) {
        // Формируем путь к файлу
        std::string file_path = current_dir + "/" + files[selected_file];
        // Получаем информацию о файле
        struct stat st;
        if (stat(file_path.c_str(), &st) == 0) {
            // Если файл является каталогом, выводим сообщение об ошибке
            if (S_ISDIR(st.st_mode)) {
                printw("Cannot open directory as a file.\n");
                refresh();
            }
            // Если файл является файлом, определяем его тип и выполняем соответствующую операцию открытия
            else {
                std::string command;
                size_t dot_pos = file_path.find_last_of(".");
                std::string extension = (dot_pos != std::string::npos) ? file_path.substr(dot_pos + 1) : "";

                // В зависимости от типа файла, формируем команду открытия
                if (extension == "txt" || extension == "cpp" || extension == "h" || extension == "c") {
                    command = "gedit ";
                } else if (extension == "pdf") {
                    command = "evince ";
                } else if (extension == "jpg" || extension == "jpeg" || extension == "png" || extension == "bmp") {
                    command = "eog ";
                } else if (extension == "mp4" || extension == "mkv" || extension == "avi") {
                    command = "vlc ";
                } else if (extension == "docx") {
                    command = "libreoffice ";
                } else if (extension.empty()) {
                    command = "gedit ";
                } else {
                    // Если тип файла не поддерживается, выводим сообщение об ошибке и выходим из функции
                    printw("Unsupported file type: %s\n", extension.c_str());
                    refresh();
                    return;
                }

                // Добавляем путь к файлу к команде открытия и выполняем ее
                command += "'" + file_path + "'";
                printw("Opening file: %s\n", command.c_str());
                refresh();
                system(command.c_str());
            }
        }
        // Если не удалось получить информацию о файле, выводим сообщение об ошибке
        else {
            printw("Error: Unable to stat the selected file.\n");
            refresh();
        }
    }
    // Если файл не выбран, выводим сообщение об ошибке
    else {
        printw("No file selected to open.\n");
        refresh();
    }
}

void FilePanel::show_file_info() {
    // Если выбран файл, выполняем операцию отображения информации о файле
    if (selected_file >= 0 && selected_file < static_cast<int>(files.size())) {
        // Формируем путь к файлу
        std::string file_path = current_dir + "/" + files[selected_file];
        // Получаем информацию о файле
        struct stat st;
        if (stat(file_path.c_str(), &st) == 0) {
            // Извлекаем информацию о файле
            std::string name = files[selected_file];
            std::string extension = name.substr(name.find_last_of(".") + 1);
            std::string access_time = ctime(&st.st_atime);
            std::string modification_time = ctime(&st.st_mtime);
            off_t size = st.st_size;

            // Создаем окно для отображения информации о файле
            int height = 10;
            int width = 60;
            int x = (COLS - width) / 2;
            int y = (LINES - height) / 2;

            WINDOW* win = newwin(height, width, y, x);
            box(win, 0, 0);

            // Выводим информацию о файле в окно
            mvwprintw(win, 1, 2, "Name: %s", name.c_str());
            mvwprintw(win, 2, 2, "Extension: %s", extension.c_str());
            mvwprintw(win, 3, 2, "Access Time: %s", access_time.c_str());
            mvwprintw(win, 4, 2, "Modification Time: %s", modification_time.c_str());
            mvwprintw(win, 5, 2, "Size: %ld bytes", size);
            mvwprintw(win, 6, 2, "Path: %s", file_path.c_str());

            // Обновляем содержимое окна и ожидаем нажатия клавиши Esc для закрытия окна
            wrefresh(win);

            int ch;
            while ((ch = getch()) != 27) {
            }

            // Удаляем окно и обновляем содержимое экрана
            delwin(win);
            refresh();
        }
        // Если не удалось получить информацию о файле, выводим сообщение об ошибке
        else {
            printw("Error: Unable to stat the selected file.\n");
            refresh();
        }
    }
    // Если файл не выбран, выводим сообщение об ошибке
    else {
        printw("No file selected to show information.\n");
        refresh();
    }
}




