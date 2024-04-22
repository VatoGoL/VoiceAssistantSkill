#pragma once
#include <functional>
#include <iostream>
#include <string>
#include <ctime>
#include <fstream>
#include <filesystem>
#include <queue>

namespace fs = std::filesystem;

class Logger
{
public:
    static const std::string DEFAULT_PATH;
    static const std::string DEFAULT_DIRECTORY;
    static const std::string PREFIX;
    static const std::string POSTFIX;
    static const size_t MAX_FILESIZE;
    static const uint8_t DEFAULT_MAX_COUNT_MESSAGE_IN_QUEUE;

    enum log_message_t
    {
        ERROR = 0,
        WARNING,
        EVENT
    };
    enum error_logger_t
    {
        FILE_NOT_OPEN = 1,
        CHECK_FILESIZE
    };

    typedef std::function<void(error_logger_t, std::string)> error_func_t;

    Logger() = default;
    ~Logger();
    static void init(uint8_t max_queue_size = DEFAULT_MAX_COUNT_MESSAGE_IN_QUEUE,
                     std::string path_to_log_directory = DEFAULT_PATH, 
                     std::string directory = DEFAULT_DIRECTORY,
                     error_func_t error_callback = std::bind(&Logger::__defaultErrorCallback,std::placeholders::_1,std::placeholders::_2));
    
    static void writeLog(std::string class_name, std::string method_name, log_message_t type_message, std::string message);
private:

    static void __writeLogQueue();
    static void __defaultErrorCallback(error_logger_t error_type, std::string message);
    static error_func_t __error_callback;
    static std::string __path_to_log_directory;
    static std::string __directory;
    static std::string __full_path_to_directory;
    static std::string __filename;

    static const uint16_t __DELTA_FILESIZE;
    static std::queue<std::string> __message_log_queue;

    static std::ofstream __fout;
    static uint8_t __max_queue_size;

    static const int __BUFFER_TIME_SIZE;
    static char __buffer_time[];
    static std::timespec __time_now;
};