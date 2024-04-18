#pragma once

#include "Modules/Server/DataBaseServer.hpp"
#define DEFINE_CONFIG "config.txt"

class ServerDataBase: public std::enable_shared_from_this<ServerDataBase>
{
private:
	std::shared_ptr<net::io_context> __ioc;
	short __countThreads;
	std::shared_ptr<Server> __server;
	std::shared_ptr<Configer> __config;
	std::shared_ptr<Logger> __log_server;
	std::string __name_config;
	std::map<std::string, std::string> __config_info;
	static const int CONFIG_NUM_FIELDS = 2;
	std::vector<std::string> CONFIG_FIELDS = { "Port", "Count_threads"};
public:
	ServerDataBase(std::string config_way, std::string name_conf);
	~ServerDataBase();
	void start();
	void stop();
};