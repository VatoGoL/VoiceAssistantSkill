#include <iostream>
#include "MarusiaWorker.hpp"

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, ".UTF-8");
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
	ServerWorker sW(config_file_way, config_file_name);
	sW.run();
}