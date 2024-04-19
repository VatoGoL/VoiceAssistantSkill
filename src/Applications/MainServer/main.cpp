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

    MainServer app;

    switch(app.init(config_file_name))
    {
        case MainServer::PROCESS_CODE::UNKNOWN_ERROR:
            std::cerr << "ERROR! app init: unknown error" << std::endl;
            return EXIT_FAILURE;
            break;
        case MainServer::PROCESS_CODE::LOG_FILE_NOT_OPEN:
            std::cerr << "ERROR! app init: log file not open" << std::endl;
            return EXIT_FAILURE;
            break;
        case MainServer::PROCESS_CODE::CONFIG_FILE_NOT_OPEN:
            std::cerr << "ERROR! app init: config file not open" << std::endl;
            return EXIT_FAILURE;
            break;
        case MainServer::PROCESS_CODE::CONFIG_DATA_NOT_FULL:
            std::cerr << "ERROR! app init: config data not full" << std::endl;
            return EXIT_FAILURE;
            break;
        case MainServer::PROCESS_CODE::CONFIG_DATA_NOT_CORRECT:
            std::cerr << "ERROR! app init: config data not correct" << std::endl;
            return EXIT_FAILURE;
            break;
        case MainServer::PROCESS_CODE::SUCCESSFUL:
            app.start();
            break;
    }
    

    return EXIT_SUCCESS;
}