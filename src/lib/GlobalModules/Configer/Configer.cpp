#include "Configer.hpp"


Configer::Configer(std::shared_ptr<Logger> lg, std::string root_directory, std::string way, std::string file_name)
{
	std::cerr << "HI" << std::endl;
	if (file_name == "") {
		__file_name = CONFIG_FILE;
	}
	setLog(lg);

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
	std::cerr << "final " << __final_path << std::endl;
	std::ifstream fin(__final_path);
	if (!fin.is_open())
	{
		__writeError(NOT_FOUND_CONFIG_FILE);
	}
	else
	{
		fin.close();
	}
}

void Configer::setLog(std::shared_ptr<Logger> lg)
{
	__log_config = lg;
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
void Configer::__writeError(int error)
{
	__log_config->writeLog(error, "Config", "open config");
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
		__log_config->writeLog(0, "config", "Write Config");
		__config_info = config;
		fin.close();
	}
	else
	{
		__writeError(NOT_FOUND_CONFIG_FILE);
		return;
	}
}
std::map<std::string, std::string> Configer::getConfigInfo()
{
	return __config_info;
}