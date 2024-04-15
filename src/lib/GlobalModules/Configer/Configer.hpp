#pragma once
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <locale.h>
#include <ctime>
#include <map>
#include <fstream>
#include <GlobalModules/Logger/Logger.hpp>

#define CONFIG_FILE "config.txt"
#define NOT_FOUND_CONFIG_FILE 1

class Configer : public std::enable_shared_from_this<Configer>
{
public:
    Configer(std::shared_ptr<Logger> lg, std::string root_directory, std::string way, std::string file_name = CONFIG_FILE);
    
    std::map<std::string, std::string>getConfigInfo();    
    void readConfig();
    void setLog(std::shared_ptr<Logger> lg);
    void setWay(std::string way);
    void setRootDirectory(std::string root_directory);
    void setFileName(std::string file_name);
private:
    
    std::string __root_directory;
    std::string __way;
    std::string __file_name;
    std::string __final_path;
    std::shared_ptr<Logger> __log_config;
    std::map<std::string, std::string> __config_info;
    void __writeError(int error);
};