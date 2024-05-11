#include "FilePanel.h"

#include <cstring>
#include <dirent.h>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ncurses.h>
#include <fcntl.h>
#include "input_window.h"
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
        : y(start_y), x(start_x), h(height), w(width), selected(false) {
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
    werase(win);
    box(win, 0, 0);
    mvwprintw(win, 0, (w - 13) / 2, "File Browser");

    mvwprintw(win, 1, 1, "Current path: %s", current_dir.c_str());

    mvwprintw(win, 2, 1, "Name");
    mvwprintw(win, 2, w / 3, "Size");
    mvwprintw(win, 2, 2 * w / 3, "Last Modified");

    wmove(win, 3, 0);
    whline(win, ACS_HLINE, w);

    if (files.empty()) {
        mvwprintw(win, h / 2, (w - 20) / 2, "No files in this directory");
    } else {
        for (size_t i = 0; i < files.size(); ++i) {
            if (static_cast<int>(i) == selected_file && selected)
                wattron(win, A_REVERSE);

            struct stat st;
            std::string file_path = current_dir + "/" + files[i];
            if (stat(file_path.c_str(), &st) == 0) {
                time_t last_modified = st.st_mtime;
                tm* time_info = localtime(&last_modified);

                std::string size = std::to_string(st.st_size);

                mvwprintw(win, i + 4, 1, "%s", files[i].c_str());
                mvwprintw(win, i + 4, w / 3, "%s", size.c_str());
                mvwprintw(win, i + 4, 2 * w / 3, "%s", asctime(time_info));
            } else {
                mvwprintw(win, i + 4, 1, "%s", files[i].c_str());
            }

            wattroff(win, A_REVERSE);
        }
    }

    if (selected_file >= 0 && selected_file < static_cast<int>(files.size())) {
        std::string selected_file_name = files[selected_file];
        mvwprintw(win, h - 3, 1, "Selected: %s", selected_file_name.c_str());
    }


    for (int i = 1; i < h + 100; ++i) {
        mvwaddch(win, i, w - 1, ACS_VLINE);
    }

    wrefresh(win);
}


void FilePanel::update() {
    list_directory();
    draw();
}

void FilePanel::move_selection(int dir) {
    selected_file += dir;
    if (selected_file < 0)
        selected_file = files.size() - 1;
    else if (selected_file >= static_cast<int>(files.size()))
        selected_file = 0;
    draw();
}

void FilePanel::change_directory(int dir) {
    if (files.empty()) {
        return;
    }

    std::string new_dir;

    if (dir == -1) { // back
        if (current_dir == "/")
            return;
        new_dir = current_dir.substr(0, current_dir.find_last_of('/'));
        if (new_dir.empty())
            new_dir = "/";
    } else if (dir == 1) { // forward
        if (selected_file >= static_cast<int>(files.size()) || files[selected_file] == ".." || files[selected_file] == ".")
            return;
        new_dir = current_dir + "/" + files[selected_file];
    }

    struct stat st;
    if (stat(new_dir.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
        current_dir = new_dir;
        strcpy(current_dir_cstr, current_dir.c_str());
        update();
    } else {
        printw("Error: cannot change directory to %s\n", new_dir.c_str());
        refresh();
    }

    printw("Dir: %d, Selected file: %d, Current dir: %s, New dir: %s\n", dir, selected_file, current_dir.c_str(), new_dir.c_str());
    refresh();
}

void FilePanel::list_directory() {
    files.clear();
    DIR* dir = opendir(current_dir_cstr);
    if (!dir)
        return;

    dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            files.insert(files.begin(), entry->d_name);
        } else {
            files.push_back(entry->d_name);
        }
    }

    closedir(dir);
}

bool FilePanel::is_selected() const {
    return selected;
}

void FilePanel::set_selected(bool selected) {
    this->selected = selected;
}

void FilePanel::create_tab() {
    // Find an empty tab slot
    for (int i = 0; i < MAX_TABS; ++i) {
        if (tabs[i].empty()) {
            // Save the current directory to the empty tab slot
            tabs[i] = current_dir;
            current_tab_index = i;
            break;
        } else if (tabs[i] == current_dir) {
            // If a tab with the same directory already exists, switch to it
            current_tab_index = i;
            return;
        }
    }
}


void FilePanel::switch_to_tab(int tab_index) {
    // Switch to the specified tab (0-9) if it exists
    if (tab_index >= 0 && tab_index < 10 && !tabs[tab_index].empty()) {
        current_dir = tabs[tab_index];
        strcpy(current_dir_cstr, current_dir.c_str());
        update();
    }
}

void FilePanel::show_tabs() {
    // Display all tabs in a separate window
    int tab_win_y = LINES - 15; // Bottom of the screen (increased by 5)
    int tab_win_x = 0; // Left of the screen
    int tab_win_h = 15; // Height of the window (increased by 5)
    int tab_win_w = COLS / 3; // One third of the screen width

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
        mvprintw(LINES - 2, 0, "Invalid tab index");
        return;
    }

    if (!isdigit(tabs[index][0])) {
        mvprintw(LINES - 2, 0, "Invalid tab index");
        return;
    }

    tabs[index] = "";
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
    new_name = input_window.show(message);

    if (new_name.empty()) {
        // Очищаем строку перед выводом сообщения об ошибке
        int y, x;
        getyx(stdscr, y, x);
        mvprintw(y, 0, " %*s", COLS, "");
        printw("New name cannot be empty. Enter new name: ");
        refresh();
        return;
    }

    // Проверяем, не совпадает ли новое имя с уже существующим
    for (const auto& file : files) {
        if (file == new_name) {
            int y, x;
            getyx(stdscr, y, x);
            mvprintw(y, 0, " %*s", COLS, "");
            printw("A file or directory with that name already exists. Enter new name: ");
            refresh();
            return;
        }
    }

    std::string old_path = current_dir + "/" + old_name;
    std::string new_path = current_dir + "/" + new_name;

    if (rename(old_path.c_str(), new_path.c_str()) == 0) {
        int y, x;
        getyx(stdscr, y, x);
        mvprintw(y, 0, " %*s", COLS, "");
        refresh();

        clearok(stdscr, TRUE);
        update();
    } else {
        int y, x;
        getyx(stdscr, y, x);
        mvprintw(y, 0, " %*s", COLS, "");
        printw("Failed to rename file or directory. Enter new name: ");
        refresh();
    }

    curs_set(0); // скрываем курсор после закрытия окна ввода
}

void FilePanel::copy_file_or_directory() {
    if (selected_file >= 0 && selected_file < static_cast<int>(files.size())) {
        copied_file_or_directory.file_path = current_dir + "/" + files[selected_file];
        copied_file_or_directory.source_panel = this;
        printw("Copied: %s\n", copied_file_or_directory.file_path.c_str());
        refresh();
    } else {
        printw("No file or directory selected to copy.\n");
        refresh();
    }
}

void FilePanel::paste_file_or_directory() {
    if (!copied_file_or_directory.file_path.empty()) {
        std::string destination = current_dir;
        struct stat st;
        if (stat(copied_file_or_directory.file_path.c_str(), &st) == 0) {
            std::string target_file = destination + "/" + copied_file_or_directory.file_path.substr(copied_file_or_directory.file_path.find_last_of('/') + 1);
            if (stat(target_file.c_str(), &st) == 0) {
                InputWindow input_window(120, 8);
                std::string message = "File already exists. Overwrite? (y/n)";
                std::string response = input_window.show(message);
                if (response == "y" || response == "Y") {
                    if (S_ISDIR(st.st_mode)) {
                        std::string command = "cp -r '" + copied_file_or_directory.file_path + "' '" + destination + "'";
                        printw("Pasting directory: %s\n", command.c_str());
                        refresh();
                        system(command.c_str());
                    } else {
                        std::string command = "cp '" + copied_file_or_directory.file_path + "' '" + destination + "'";
                        printw("Pasting file: %s\n", command.c_str());
                        refresh();
                        system(command.c_str());
                    }
                    update();
                }
            } else {
                if (S_ISDIR(st.st_mode)) {
                    std::string command = "cp -r '" + copied_file_or_directory.file_path + "' '" + destination + "'";
                    printw("Pasting directory: %s\n", command.c_str());
                    refresh();
                    system(command.c_str());
                } else {
                    std::string command = "cp '" + copied_file_or_directory.file_path + "' '" + destination + "'";
                    printw("Pasting file: %s\n", command.c_str());
                    refresh();
                    system(command.c_str());
                }
                update();
            }
        } else {
            printw("Error: Unable to stat the copied file or directory.\n");
            refresh();
        }
    } else {
        printw("Error: Nothing has been copied to paste.\n");
        refresh();
    }
}

void FilePanel::open_file() {
    if (selected_file >= 0 && selected_file < static_cast<int>(files.size())) {
        std::string file_path = current_dir + "/" + files[selected_file];
        struct stat st;
        if (stat(file_path.c_str(), &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                printw("Cannot open directory as a file.\n");
                refresh();
            } else {
                std::string command;
                std::string extension = file_path.substr(file_path.find_last_of(".") + 1);

                if (extension == "txt" || extension == "cpp" || extension == "h" || extension == "c") {
                    command = "gedit ";
                } else if (extension == "pdf") {
                    command = "evince ";
                } else if (extension == "jpg" || extension == "jpeg" || extension == "png" || extension == "bmp") {
                    command = "eog ";
                } else if (extension == "mp4" || extension == "mkv" || extension == "avi") {
                    command = "vlc ";
                } else {
                    printw("Unsupported file type: %s\n", extension.c_str());
                    refresh();
                    return;
                }

                command += "'" + file_path + "'";
                printw("Opening file: %s\n", command.c_str());
                refresh();
                system(command.c_str());
            }
        } else {
            printw("Error: Unable to stat the selected file.\n");
            refresh();
        }
    } else {
        printw("No file selected to open.\n");
        refresh();
    }
}

