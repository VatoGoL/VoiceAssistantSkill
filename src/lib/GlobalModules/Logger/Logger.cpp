#include "Logger.hpp"

const std::string Logger::DEFAULT_PATH = "./";
const std::string Logger::DEFAULT_DIRECTORY = "Logs";
const std::string Logger::PREFIX = "log";
const std::string Logger::POSTFIX = ".log";
const size_t Logger::MAX_FILESIZE = 1024*100; //100Kb
const uint16_t Logger::__DELTA_FILESIZE = 1024*2; //2Kb
std::string Logger::__full_path_to_directory = DEFAULT_PATH+DEFAULT_DIRECTORY;
std::ofstream Logger::__fout(__full_path_to_directory + PREFIX + "_notInit" + POSTFIX);
const uint8_t Logger::DEFAULT_MAX_COUNT_MESSAGE_IN_QUEUE = 10;
Logger::error_func_t Logger::__error_callback = {};
std::string Logger::__path_to_log_directory;
std::string Logger::__directory;
std::string Logger::__filename;
uint8_t Logger::__max_queue_size;
std::queue<std::string> Logger::__message_log_queue;
const int Logger::__BUFFER_TIME_SIZE = sizeof("yyyy_mm_dd_hh_mm_ss_");
char Logger::__buffer_time[Logger::__BUFFER_TIME_SIZE];
std::timespec Logger::__time_now;

void Logger::init(uint8_t max_queue_size, std::string path_to_log_directory, std::string directory, error_func_t error_callback)
{
	__fout.close();
	__max_queue_size = max_queue_size;
	__error_callback = error_callback;
	__path_to_log_directory = path_to_log_directory;
	__directory = directory;
	__full_path_to_directory = __path_to_log_directory + __directory;

	if(!fs::exists(__full_path_to_directory)){
		fs::create_directory(directory);
	}
	std::timespec_get(&__time_now, TIME_UTC);
	std::strftime(__buffer_time,__BUFFER_TIME_SIZE,"%Y_%m_%d_%H_%M_%S_",std::gmtime(&__time_now.tv_sec));
	__filename = __full_path_to_directory + "/" + PREFIX + "_" 
				 + std::string(__buffer_time) + std::to_string(__time_now.tv_nsec / 1'000'000)
				 + POSTFIX;
	if(std::filesystem::exists(__filename)){
		__fout.open(__filename, std::ofstream::app);
	}else{
		__fout.open(__filename);
	}
	
	if(!__fout.is_open()){
		throw error_logger_t::FILE_NOT_OPEN;
	}
	__fout.close();
}
void Logger::writeLog(std::string class_name, std::string method_name, log_message_t type_message, std::string message)
{
	std::string type_message_str;
	switch(type_message){
		case ERROR:
			type_message_str = "ERROR";
		break;
		case WARNING:
			type_message_str = "WARNING";
		break;
		case EVENT:
			type_message_str = "EVENT";
		break;
	}
	std::timespec_get(&__time_now, TIME_UTC);
			std::strftime(__buffer_time,__BUFFER_TIME_SIZE,"%Y_%m_%d_%H_%M_%S",std::gmtime(&__time_now.tv_sec));
	std::string end_message = "[" + std::string(__buffer_time) + "]["
							  + type_message_str + "] Class: " + class_name + "; Method: " + method_name + "; Message: "
							  + message;	
	__message_log_queue.push(end_message);
	
	if(__message_log_queue.size() >= __max_queue_size){
		__writeLogQueue();
	}
}
void Logger::__writeLogQueue()
{
	try{
		if(fs::file_size(__filename) > MAX_FILESIZE - __DELTA_FILESIZE)
		{
			std::timespec_get(&__time_now, TIME_UTC);
			std::strftime(__buffer_time,__BUFFER_TIME_SIZE,"%Y_%m_%d_%H_%M_%S_",std::gmtime(&__time_now.tv_sec));
			__filename =  __full_path_to_directory + "/" +  PREFIX + "_" 
				 + std::string(__buffer_time) + std::to_string(__time_now.tv_nsec / 1'000'000)
				 + POSTFIX;
			
		}
	}catch(std::exception &e){
		__error_callback(CHECK_FILESIZE, e.what());
		return;
	}
	if(std::filesystem::exists(__filename)){
		__fout.open(__filename, std::ofstream::app);
	}else{
		__fout.open(__filename);
	}
	if(!__fout.is_open()){
		__error_callback(FILE_NOT_OPEN, "log file" + __filename + "not open");
		return;
	}

	while(!__message_log_queue.empty())
	{
		__fout << __message_log_queue.front() << std::endl;
		__message_log_queue.pop();
	}

	__fout.close();
}
void Logger::__defaultErrorCallback(error_logger_t error_type, std::string message)
{
	switch(error_type){
		case FILE_NOT_OPEN:
			std::cerr << "ERROR! Logger - __defaultErrorCallback: file not open, message: " + message << std::endl;
			break;
	}
}
Logger::~Logger()
{
	__writeLogQueue();
}