#include "file_operations.h"
#include "input_window.h"
#include <iostream>
#include <string>
#include <experimental/filesystem>
#include <fstream>

namespace fs = std::experimental::filesystem;

// Функция для создания файла
void create_file(const std::string &path) {
    // Проверяем, существует ли ресурс с таким же именем
    if (fs::exists(path)) {
        // Если ресурс является каталогом, запрашиваем у пользователя разрешение на удаление
        if (fs::is_directory(path)) {
            InputWindow input_window(120, 8);
            std::string message = "Resourse with the same name exists. Delete it? (y/n)";
            std::string response = input_window.show(message);
            if (response == "y" || response == "Y") {
                fs::remove_all(path);
            } else {
                return;
            }
        }
        // Если ресурс является файлом, запрашиваем у пользователя разрешение на перезапись
        else {
            InputWindow input_window(120, 8);
            std::string message = "Resourse already exists. Overwrite? (y/n)";
            std::string response = input_window.show(message);
            if (response != "y" && response != "Y") {
                return;
            }
        }
    }

    // Создаем файл
    std::ofstream file(path, std::ios::trunc);
    // Если не удалось создать файл, выводим сообщение об ошибке
    if (!file) {
        printw("Error: Unable to create resourse.\n");
        refresh();
    }
}

// Функция для создания каталога
void create_directory(const std::string &path) {
    // Проверяем, существует ли ресурс с таким же именем
    if (fs::exists(path)) {
        // Если ресурс существует, запрашиваем у пользователя разрешение на перезапись
        InputWindow input_window(120, 8);
        std::string message = "Resourse already exists. Overwrite? (y/n)";
        std::string response = input_window.show(message);
        if (response == "y" || response == "Y") {
            // Если пользователь разрешил перезапись, удаляем существующий ресурс и создаем новый каталог
            std::string command = "rm -r '" + path + "' && mkdir '" + path + "'";
            system(command.c_str());
        }
    } else {
        // Если ресурс не существует, создаем новый каталог
        std::string command = "mkdir '" + path + "'";
        system(command.c_str());
    }
}
