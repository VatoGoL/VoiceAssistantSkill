#include "Configer.hpp"

const std::string Configer::CONFIG_FILE = "config.txt";
Configer::error_func_t Configer::__callback;
std::string Configer::__root_directory;
std::string Configer::__way;
std::string Configer::__file_name;
std::string Configer::__final_path;
std::map<std::string, std::string> Configer::__config_info;

void Configer::init(std::string root_directory, std::string way, std::string file_name, error_func_t callback)
{
	__callback = callback;
	if (file_name == "") {
		__file_name = CONFIG_FILE;
	}

	__root_directory = root_directory;
	__way = way;
	if (file_name != "")
	{
		__file_name = file_name;
	}
	else
	{
		__file_name = CONFIG_FILE;
	}
	__final_path = __root_directory + __way +  __file_name;
	std::ifstream fin(__final_path);
	if (!fin.is_open())
	{
		throw error_configer_t::FILE_NOT_OPEN;
	}
	else
	{
		fin.close();
	}
}

void Configer::setWay(std::string way)
{
	__way = way;
	__final_path = __way + __root_directory + __file_name;
}
void Configer::setRootDirectory(std::string root_directory) {
	__root_directory = root_directory;
	__final_path = __way + __root_directory + __file_name;
}
void Configer::setFileName(std::string file_name) {
	__file_name = file_name;
	__final_path = __way + __root_directory + __file_name;
}

void Configer::readConfig()
{
	std::ifstream fin(__final_path);
	if (fin.is_open())
	{
		std::map <std::string, std::string> config;
		while (!fin.eof())
		{
			std::string boof;
			std::string key;
			getline(fin, boof);
			size_t border = boof.find(":");
			key.append(boof, 0, border);
			boof.erase(0, border + 1);
			border = boof.find("\r");
			if(border != std::string::npos){
				boof.erase(border,1);
			}
			config.insert(std::pair<std::string, std::string>(key, boof));
		}
		__config_info = config;
		fin.close();
	}
	else
	{
		Logger::writeLog("Configer", "readConfig", Logger::log_message_t::ERROR, "config file not open");
		__callback(FILE_NOT_OPEN, "config file not open");
		return;
	}
}
std::map<std::string, std::string>& Configer::getConfigInfo()
{
	return __config_info;
}
void Configer::__defaultErrorCallback(error_configer_t error_type, std::string message)
{
	switch(error_type)
	{
		case FILE_NOT_OPEN:
			std::cerr << "ERROR! Configer - __defaultErrorCallback, message: " << message << std::endl;
			break;
	}
}