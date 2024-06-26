#include <iostream>
#include "SQLiteWebWrapper.hpp"

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "ru_RU.UTF-8");
    std::string config_file_way = "";
    std::string config_file_name = "";
    for (int i = 1; i < argc; i++) {
        std::string flags = argv[i];
        if (flags == "-cf" || flags == "--config_file") {
            config_file_way = argv[++i];
        }
        else if (flags == "-cf_n" || flags == "--config_file_name")
        {
            config_file_name = argv[++i];
        }
    }
	ServerDataBase app;
    bool exit_fail = false;
    switch(app.init(config_file_way, config_file_name)){
        case ServerDataBase::PROCESS_CODE::UNKNOWN_ERROR:
            std::cerr << "ERROR! app init: unknown error" << std::endl;
            exit_fail = true;
            break;
        case ServerDataBase::PROCESS_CODE::LOG_FILE_NOT_OPEN:
            std::cerr << "ERROR! app init: log file not open" << std::endl;
            exit_fail = true;
            break;
        case ServerDataBase::PROCESS_CODE::CONFIG_FILE_NOT_OPEN:
            std::cerr << "ERROR! app init: config file not open" << std::endl;
            exit_fail = true;
            break;
        case ServerDataBase::PROCESS_CODE::CONFIG_DATA_NOT_FULL:
            std::cerr << "ERROR! app init: config data not full" << std::endl;
            exit_fail = true;
            break;
        case ServerDataBase::PROCESS_CODE::CONFIG_DATA_NOT_CORRECT:
            std::cerr << "ERROR! app init: config data not correct" << std::endl;
            exit_fail = true;
            break;
        case ServerDataBase::PROCESS_CODE::SUCCESSFUL:
            app.start();
        break;
    }
	
    return exit_fail ? EXIT_SUCCESS : EXIT_FAILURE;
}