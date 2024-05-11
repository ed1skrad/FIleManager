#ifndef FILEPANEL_H
#define FILEPANEL_H

#include <string>
#include <vector>
#include <cstring>
#include <dirent.h>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ncurses.h>
#include <sys/types.h>
#include <fcntl.h>

class FilePanel;

struct CopiedFile {
    std::string file_path;
    FilePanel* source_panel;
};

extern CopiedFile copied_file_or_directory;

class FilePanel {
public:
    FilePanel(int start_y, int start_x, int height, int width);
    ~FilePanel();
    void draw();
    void update();
    void move_selection(int dir);
    void change_directory(int dir);
    bool is_selected() const;
    void set_selected(bool selected);
    int get_selected_file_index() const;
    int get_files_size() const;
    std::string get_current_dir() const;
    std::string get_selected_file() const;
    void delete_tab(int index);
    void create_tab();
    void switch_to_tab(int tab_index);
    void show_tabs();
    std::string get_tab_by_index(int index) const;
    void set_current_tab_index(int index);
    int get_current_tab_index() const;
    void rename_file_or_directory();
    void copy_file_or_directory();
    void paste_file_or_directory();
    void open_file();
    void set_size(int height, int width) {
        h = height;
        w = width;
        werase(win);
        wresize(win, h, w);
    }

    void set_position(int y, int x) {
        this->y = y;
        this->x = x;
        mvwin(win, y, x);
    }
private:
    int y, x, h, w;
    WINDOW* win;
    std::vector<std::string> files;
    int selected_file;
    std::string current_dir;
    char current_dir_cstr[PATH_MAX];
    bool selected;
    std::string tabs[10];
    int current_tab_index;
    void list_directory();
};

#endif // FILEPANEL_H
