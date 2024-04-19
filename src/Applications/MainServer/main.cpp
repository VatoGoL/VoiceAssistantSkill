#include <iostream>
#include <string>

#include "MainServer.hpp"

int main(int argc, char* argv[])
{
    setlocale(LC_ALL, ".UTF-8");

    std::string config_file_name = "";
    for (int i = 1; i < argc; i++) {
        std::string flags = argv[i];
        if (flags == "-cf" || flags == "--config_file") {
            config_file_name = argv[++i];
        }
    }

    MainServer ms;
    ms.init(config_file_name);
    ms.start();

    return EXIT_SUCCESS;
}