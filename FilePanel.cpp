#include "FilePanel.h"
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
    list_directory();
}

FilePanel::~FilePanel() {
    delwin(win);
}


void FilePanel::draw() {
    werase(win);//очиска содержимого окна
    box(win, 0, 0);
    mvwprintw(win, 0, (w - 13) / 2, "File Browser");

    std::string current_dir_display = current_dir;
    if (current_dir_display.back() == '/' && current_dir_display.size() > 1) {
        current_dir_display.pop_back(); //обработка лишнего 
    }
    mvwprintw(win, 1, 1, "Current path: %s", current_dir_display.c_str());

    mvwprintw(win, 2, w / 3, "Size");
    mvwprintw(win, 2, 2 * w / 3, "Last Modified");

    wmove(win, 3, 0);
    whline(win, ACS_HLINE, w); // отрисовка разделительной линии между информацией

    if (files.empty()) {
        mvwprintw(win, h / 2, (w - 20) / 2, "No files in this directory"); //выво д сообщения
    } else {
        int start_index = scroll_position; 
        int end_index = std::min(scroll_position + h - 5, static_cast<int>(files.size()));//вычесление области видимости файлов
        for (int i = start_index; i < end_index; ++i) {
            if (i == selected_file && selected) //выделение файла или вшк
                wattron(win, A_REVERSE);//белый цвет

            struct stat st; //создание структуры для получения информации о файле или директории
            std::string file_path = current_dir + "/" + files[i];//формируем полный путь к файлу
            if (stat(file_path.c_str(), &st) == 0) {// если информация о файле может быть получена
                time_t last_modified = st.st_mtime;//время последней модификации
                tm* time_info = localtime(&last_modified);

                std::string size = std::to_string(st.st_size);//получения размера в байтах и преобр в str

                /*
                *Вывод данной информации на экран
                */
                mvwprintw(win, i - scroll_position + 4, 1, "%s", files[i].c_str());
                mvwprintw(win, i - scroll_position + 4, w / 3, "%s", size.c_str());
                mvwprintw(win, i - scroll_position + 4, 2 * w / 3, "%s", asctime(time_info));
            } else {
                mvwprintw(win, i - scroll_position + 4, 1, "%s", files[i].c_str());
            }

            wattroff(win, A_REVERSE);//выключаем обратные цвета для файла в списке
        }
    }

    if (selected_file >= 0 && selected_file < static_cast<int>(files.size())) { //если файл находится в предеах
        std::string selected_file_name = files[selected_file];//получаем его имя
        mvwprintw(win, 2, 1, "Selected: %s", selected_file_name.c_str());//выводим
    }

    for (int i = 1; i < h + 100; ++i) {
        mvwaddch(win, i, w - 1, ACS_VLINE); //графическая часть
    }

    wrefresh(win);//обновляем содержимое на экране
}

void FilePanel::update() {
    list_directory();
    scroll_position = 0;
    max_scroll_position = std::max(0, static_cast<int>(files.size()) - h + 5);
    draw();
}

void FilePanel::move_selection(int dir) {
    selected_file += dir; //изменение индекса выделенного элемента на dir(+1 -1)
    if (selected_file < 0)
        selected_file = files.size() - 1; // если индекс меньше 0, то элемент последний в списке
    else if (selected_file >= static_cast<int>(files.size())) // иначе если превышает кол.во элементов в списке то он становиться первым
        selected_file = 0;

    if (selected_file < scroll_position) //реализация прокрутки, если элемент выше позиции, то он становится индексным элементом для проекрутки
        scroll_position = selected_file;
    else if (selected_file >= scroll_position + h - 5)//это означает, что выделенный элемент находится ниже видимой области окна просмотра
        scroll_position = selected_file - h + 6;//показывем файл

    draw();//функция отрисовки
}

void FilePanel::change_directory(int dir) {
    // Если список файлов пуст, то ничего не делаем
    if (files.empty()) {
        return;
    }

    std::string new_dir;

    // Если dir равен -1, то переходим в родительскую директорию
    if (dir == -1) {
        // Если текущая директория является корневой, то ничего не делаем
        if (current_dir == "/")
            return;
        // Извлекаем путь к родительской директории
        new_dir = current_dir.substr(0, current_dir.find_last_of('/'));
        // Если путь к родительской директории пуст, то устанавливаем его как корневую директорию
        if (new_dir.empty())
            new_dir = "/";
    }
    // Если dir равен 1, то переходим в выбранную директорию
    else if (dir == 1) {
        // Если выбранный элемент выходит за пределы списка или  ".", то ничего не делаем
        if (selected_file >= static_cast<int>(files.size()) || files[selected_file] == "..")
            return;
        // Создаем путь к выбранной директории
        new_dir = current_dir;
        if (new_dir != "/") {
            new_dir += "/";
        }
        new_dir += files[selected_file];
    }

    // Проверяем, является ли указанная директория существующей
    struct stat st;
    if (stat(new_dir.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
        // Если директория существует, то обновляем текущую директорию и список файлов
        current_dir = new_dir;
        strcpy(current_dir_cstr, current_dir.c_str());
        update();
    } else {
        // Если директория не существует, то выводим сообщение об ошибке
        printw("Error: cannot change directory to %s\n", new_dir.c_str());
        refresh();
    }

    // Выводим отладочную информацию
    printw("Dir: %d, Selected file: %d, Current dir: %s, New dir: %s\n", dir, selected_file, current_dir.c_str(), new_dir.c_str());
    refresh();
}

void FilePanel::list_directory() {
    // Очищаем вектор файлов для последующего заполнения
    files.clear();

    // Открываем директорию, указанную в текущей_директории, с помощью функции opendir
    // Если директория не может быть открыта, функция возвращает NULL
    DIR* dir = opendir(current_dir_cstr);
    if (!dir) {
        // Выходим из функции, если директория не может быть открыта
        return;
    }

    // Объявляем указатель на структуру dirent для последующего чтения информации о файлах
    dirent* entry;

    // Чтение информации о файлах в цикле, пока функция readdir не вернет NULL
    // (т.е. пока все файлы в директории не будут прочитаны)
    while ((entry = readdir(dir)) != nullptr) {
        // Сравниваем имя текущего файла (entry->d_name) со строкой ".."
        // Если это имя родительской директории, то добавляем его в начало вектора файлов
        if (strcmp(entry->d_name, "..") == 0) {
            files.insert(files.begin(), entry->d_name);
        }
        // В противном случае, сравниваем имя текущего файла со строкой "."
        // Если это не имя текущей директории, то добавляем его в конец вектора файлов
        else if (strcmp(entry->d_name, ".") != 0) {
            files.push_back(entry->d_name);
        }
    }

    // Закрываем директорию с помощью функции closedir
    closedir(dir);
}

bool FilePanel::is_selected() const {//getter
    return selected;
}

void FilePanel::set_selected(bool selected) {//setter
    this->selected = selected;
}

void FilePanel::create_tab() { //Создание вкладки
    for (int i = 0; i < MAX_TABS; ++i) {
        if (tabs[i].empty()) {
            tabs[i] = current_dir;
            current_tab_index = i;
            break;
        } else if (tabs[i] == current_dir) {
            current_tab_index = i;
            return;
        }
    }
}


void FilePanel::switch_to_tab(int tab_index) { //переключение на вкладку
    if (tab_index >= 0 && tab_index < 10 && !tabs[tab_index].empty()) {
        current_dir = tabs[tab_index];
        strcpy(current_dir_cstr, current_dir.c_str());
        update();
    }
}

void FilePanel::show_tabs() { //отображение списка вкладок
    int tab_win_y = LINES - 15; 
    int tab_win_x = 0; 
    int tab_win_h = 15; 
    int tab_win_w = COLS / 3; 

    WINDOW* tab_win = newwin(tab_win_h, tab_win_w, tab_win_y, tab_win_x);
    werase(tab_win);
    box(tab_win, 0, 0);

    for (int i = 0; i < 10; ++i) {
        if (!tabs[i].empty()) {
            mvwprintw(tab_win, i + 1, 1, "%d: %s", i, tabs[i].c_str());
        }
    }

    wrefresh(tab_win);
    getch();
    werase(tab_win);
    wrefresh(tab_win);
    delwin(tab_win);
}

void FilePanel::delete_tab(int index) {
    if (index < 0 || index >= MAX_TABS) {
        return;
    }

    if (tabs[index].empty()) {
        return;
    }

    tabs[index] = "";

    if (index == current_tab_index) {
        current_tab_index = 0;
        change_directory(0);
    } else if (index < current_tab_index) {
        current_tab_index--;
    }
}

std::string FilePanel::get_tab_by_index(int index) const { //геттер для dir вклакди
    if (index >= 0 && index < 9) {
        return tabs[index];
    }
    return "";
}

void FilePanel::set_current_tab_index(int index) { //установка dir для вкладки
    if (index >= 0 && index < 9) {
        current_tab_index = index;
        change_directory(0);
    }
}

int FilePanel::get_current_tab_index() const {//геттер для индекса вкладки
    return current_tab_index;
}

void FilePanel::rename_file_or_directory() {
    int selected_file_index = get_selected_file_index();
    if (selected_file_index == -1) {
        printw("No file or directory selected");
        refresh();
        return;
    }

    std::string old_name = get_selected_file();
    std::string new_name;

    InputWindow input_window(120, 8);
    std::string message = "Enter new name for " + old_name + ":";

    while (true) {
        new_name = input_window.show(message);

        if (new_name.empty() || std::find_if(files.begin(), files.end(),
                                             [&new_name](const std::string& file) { //проверка на существование файла с таким же именем
                                                 return file == new_name;
                                             }) != files.end()) {
            message = "Invalid name. Enter new name: ";
            continue;
        }

        break;
    }

    std::string old_path = current_dir + "/" + old_name;
    std::string new_path = current_dir + "/" + new_name;

    if (rename(old_path.c_str(), new_path.c_str()) == 0) { //если rename успешон прошел
        clearok(stdscr, TRUE);
        update();
    } else {
        message = "Failed to rename file or directory. Enter new name: ";
    }

    curs_set(0);
}

void FilePanel::copy_file_or_directory() {
    if (selected_file >= 0 && selected_file < static_cast<int>(files.size())) { //проверка на выбранный файл
        copied_file_or_directory.file_path = current_dir + "/" + files[selected_file];//запись в структуру
        copied_file_or_directory.source_panel = this; //запись указателя на текущий объект
        printw("Copied: %s\n", copied_file_or_directory.file_path.c_str());
        refresh();
    } else {
        printw("No file or directory selected to copy.\n");
        refresh();
    }
}

void FilePanel::paste_file_or_directory() {
    // проверяем, что путь к скопированному файлу или директории не пустой
    if (!copied_file_or_directory.file_path.empty()) {
        // задаем путь к месту назначения (текущая директория)
        std::string destination = current_dir;
        // создаем объект stat для получения информации о файле или директории
        struct stat st;
        // проверяем, что скопированный файл или директория существует
        if (stat(copied_file_or_directory.file_path.c_str(), &st) == 0) {
            // задаем путь к файлу, который будет создан при вставке
            std::string target_file = destination + "/" + copied_file_or_directory.file_path.substr(copied_file_or_directory.file_path.find_last_of('/') + 1);
            // проверяем, что файл с таким именем уже существует в месте назначения
            if (stat(target_file.c_str(), &st) == 0) {
                // создаем окно для ввода ответа пользователя
                InputWindow input_window(120, 8);
                // задаем сообщение для отображения в окне
                std::string message = "File already exists. Overwrite? (y/n)";
                // отображаем окно и получаем ответ пользователя
                std::string response = input_window.show(message);
                // проверяем, что пользователь согласен на замену файла
                if (response == "y" || response == "Y") {
                    // проверяем, что скопированный объект является директорией
                    if (S_ISDIR(st.st_mode)) {
                        // задаем команду для копирования директории рекурсивно
                        std::string command = "cp -r '" + copied_file_or_directory.file_path + "' '" + destination + "'";
                        // отображаем сообщение о копировании директории
                        printw("Pasting directory: %s\n", command.c_str());
                        // обновляем экран
                        refresh();
                        // выполняем команду копирования
                        system(command.c_str());
                    } else {
                        // задаем команду для копирования файла
                        std::string command = "cp '" + copied_file_or_directory.file_path + "' '" + destination + "'";
                        // отображаем сообщение о копировании файла
                        printw("Pasting file: %s\n", command.c_str());
                        // обновляем экран
                        refresh();
                        // выполняем команду копирования
                        system(command.c_str());
                    }
                    // обновляем список файлов и директорий в текущей директории
                    update();
                }
            } else {
                // проверяем, что скопированный объект является директорией
                if (S_ISDIR(st.st_mode)) {
                    // задаем команду для копирования директории рекурсивно
                    std::string command = "cp -r '" + copied_file_or_directory.file_path + "' '" + destination + "'";
                    // отображаем сообщение о копировании директории
                    printw("Pasting directory: %s\n", command.c_str());
                    // обновляем экран
                    refresh();
                    // выполняем команду копирования
                    system(command.c_str());
                } else {
                    // задаем команду для копирования файла
                    std::string command = "cp '" + copied_file_or_directory.file_path + "' '" + destination + "'";
                    // отображаем сообщение о копировании файла
                    printw("Pasting file: %s\n", command.c_str());
                    // обновляем экран
                    refresh();
                    // выполняем команду копирования
                    system(command.c_str());
                }
                // обновляем список файлов и директорий в текущей директории
                update();
            }
        } else {
            // отображаем сообщение об ошибке при получении информации о файле или директории
            printw("Error: Unable to stat the copied file or directory.\n");
            // обновляем экран
            refresh();
        }
    } else {
        // отображаем сообщение об ошибке при отсутствии скопированного файла или директории
        printw("Error: Nothing has been copied to paste.\n");
        // обновляем экран
        refresh();
    }
}


// Определение метода FilePanel::open_file()
void FilePanel::open_file() {
    // Проверка того, что выбранный файл существует в списке files
    if (selected_file >= 0 && selected_file < static_cast<int>(files.size())) {
        // Создание строки file_path, которая содержит путь к выбранному файлу
        std::string file_path = current_dir + "/" + files[selected_file];
        // Создание структуры stat для хранения информации о файле
        struct stat st;
        // Вызов функции stat для получения информации о файле по его пути
        if (stat(file_path.c_str(), &st) == 0) {
            // Проверка того, что выбранный файл не является директорией
            if (S_ISDIR(st.st_mode)) {
                // Вывод сообщения об ошибке, если выбранная директория
                printw("Cannot open directory as a file.\n");
                refresh();
            } else {
                // Создание строки command для хранения команды запуска приложения для открытия файла
                std::string command;
                // Нахождение позиции последней точки в строке file_path для извлечения расширения файла
                size_t dot_pos = file_path.find_last_of(".");
                // Создание строки extension, которая содержит расширение файла
                std::string extension = (dot_pos != std::string::npos) ? file_path.substr(dot_pos + 1) : "";

                // Установка команды запуска приложения в зависимости от расширения файла
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
                    // Вывод сообщения об ошибке, если расширение файла не поддерживается
                    printw("Unsupported file type: %s\n", extension.c_str());
                    refresh();
                    return;
                }

                // Добавление пути к файлу в команду запуска приложения
                command += "'" + file_path + "'";
                // Вывод сообщения о том, что файл будет открыт
                printw("Opening file: %s\n", command.c_str());
                refresh();
                // Вызов функции system для запуска команды запуска приложения
                system(command.c_str());
            }
        } else {
            // Вывод сообщения об ошибке, если не удалось получить информацию о файле
            printw("Error: Unable to stat the selected file.\n");
            refresh();
        }
    } else {
        // Вывод сообщения об ошибке, если не выбран файл для открытия
        printw("No file selected to open.\n");
        refresh();
    }
}


void FilePanel::show_file_info() {
    if (selected_file >= 0 && selected_file < static_cast<int>(files.size())) {
        std::string file_path = current_dir + "/" + files[selected_file];
        struct stat st;
        if (stat(file_path.c_str(), &st) == 0) {
            std::string name = files[selected_file];
            std::string extension = name.substr(name.find_last_of(".") + 1);
            std::string access_time = ctime(&st.st_atime);
            std::string modification_time = ctime(&st.st_mtime);
            off_t size = st.st_size;

            int height = 10;
            int width = 60;
            int x = (COLS - width) / 2;
            int y = (LINES - height) / 2;

            WINDOW* win = newwin(height, width, y, x);
            box(win, 0, 0);

            mvwprintw(win, 1, 2, "Name: %s", name.c_str());
            mvwprintw(win, 2, 2, "Extension: %s", extension.c_str());
            mvwprintw(win, 3, 2, "Access Time: %s", access_time.c_str());
            mvwprintw(win, 4, 2, "Modification Time: %s", modification_time.c_str());
            mvwprintw(win, 5, 2, "Size: %ld bytes", size);
            mvwprintw(win, 6, 2, "Path: %s", file_path.c_str());

            wrefresh(win);

            int ch;
            while ((ch = getch()) != 27) {
            }

            delwin(win);
            refresh();
        } else {
            printw("Error: Unable to stat the selected file.\n");
            refresh();
        }
    } else {
        printw("No file selected to show information.\n");
        refresh();
    }
}



