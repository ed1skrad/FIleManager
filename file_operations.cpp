#include "file_operations.h"
#include "input_window.h"
#include <iostream>
#include <string>
#include <experimental/filesystem>
#include <fstream>

namespace fs = std::experimental::filesystem;

void create_file(const std::string &path) {
    if (fs::exists(path)) {
        InputWindow input_window(120, 8);
        std::string message = "File already exists. Overwrite? (y/n)";
        std::string response = input_window.show(message);
        if (response == "y" || response == "Y") {
            std::ofstream file(path, std::ios::trunc);
            if (!file) {
                printw("Error: Unable to create file.\n");
                refresh();
            }
        }
    } else {
        std::ofstream file(path);
        if (!file) {
            printw("Error: Unable to create file.\n");
            refresh();
        }
    }
}

void create_directory(const std::string &path) {
    if (fs::exists(path)) {
        InputWindow input_window(120, 8);
        std::string message = "Directory already exists. Overwrite? (y/n)";
        std::string response = input_window.show(message);
        if (response == "y" || response == "Y") {
            std::string command = "rm -r '" + path + "' && mkdir '" + path + "'";
            system(command.c_str());
        }
    } else {
        std::string command = "mkdir '" + path + "'";
        system(command.c_str());
    }
}
