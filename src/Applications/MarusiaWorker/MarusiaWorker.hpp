#pragma once

#include "Modules/Worker/WorkerM.hpp"
#define DEFINE_CONFIG "config.txt"

class MarusiaWorker : public std::enable_shared_from_this<MarusiaWorker>
{
public:
	MarusiaWorker(std::string way_conf_file, std::string name_conf_file);
	void run();
	void stop();
	~MarusiaWorker();
private:
	std::shared_ptr<Logger> __log_server;
	std::shared_ptr<Configer> __config;
	std::map<std::string, std::string> __config_info;
	std::shared_ptr<tcp::socket> __sock;
	static const int CONFIG_NUM_FIELDS = 8;
	std::vector<std::string> CONFIG_FIELDS = { "Id", "Main_server_ip", "Main_server_port", "BD_ip", "BD_port", "BD_login", "BD_password", "Count_threads"};
	int __count_threads;
	std::shared_ptr<net::io_context> __ioc;
	std::shared_ptr<WorkerM> __session;
	std::string name_config;
};