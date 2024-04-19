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




class Configer
{
public:
    static const std::string CONFIG_FILE;
    enum error_configer_t{
        FILE_NOT_OPEN = 1
    };
    typedef std::function<void(error_configer_t, std::string)> error_func_t;

    Configer() = default;

    static void init(std::string root_directory, std::string way, 
                     std::string file_name = CONFIG_FILE, error_func_t callback = std::bind(&Configer::__defaultErrorCallback,std::placeholders::_1,std::placeholders::_2));
    static std::map<std::string, std::string>& getConfigInfo();    
    static void readConfig();
    static void setWay(std::string way);
    static void setRootDirectory(std::string root_directory);
    static void setFileName(std::string file_name);
    
private:
    static error_func_t __callback;
    static std::string __root_directory;
    static std::string __way;
    static std::string __file_name;
    static std::string __final_path;
    static std::map<std::string, std::string> __config_info;

    static void __defaultErrorCallback(error_configer_t error_type, std::string message);
};