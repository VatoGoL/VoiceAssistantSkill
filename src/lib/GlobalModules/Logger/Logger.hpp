#pragma once
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <locale.h>
#include <ctime>
#include <map>
#include <memory>
#include <fstream>

class Logger : public std::enable_shared_from_this<Logger>
{
public:
    Logger(std::string way, std::string root_directory, std::string name_class);
    void writeTempLog(int error, std::string clas, std::string message);
    void writeLog(int error, std::string clas, std::string message);
    void setFinalPath();
    std::streamsize getFileSize();
    std::string getDate();
private:
    bool __checkFile();
    void __writeLogToFile(std::string clas, std::string message, int error);
    std::string __name_file;
    std::string __final_path;
    std::string __date;
    int __num_file;
    int __num_massive;
    std::string __temporary_log[3];
    std::string __way;
    std::string __buf_way;
    std::string __root_directory;
};