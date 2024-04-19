#pragma once

#include "Modules/Worker/WorkerM.hpp"
#include <GlobalModules/Logger/Logger.hpp>
#include <GlobalModules/Configer/Configer.hpp>


class MarusiaWorker : public std::enable_shared_from_this<MarusiaWorker>
{
public:
	enum PROCESS_CODE {
		SUCCESSFUL = 0,
		UNKNOWN_ERROR,
		LOG_FILE_NOT_OPEN,
		CONFIG_FILE_NOT_OPEN,
		CONFIG_DATA_NOT_FULL,
		CONFIG_DATA_NOT_CORRECT
	};
	MarusiaWorker() = default;
	PROCESS_CODE init(std::string way_to_conf_file = "./", std::string name_conf_file = Configer::CONFIG_FILE);
	void run();
	void stop();
	~MarusiaWorker();
private:
	
	std::shared_ptr<tcp::socket> __sock;
	static const int CONFIG_NUM_FIELDS = 8;
	std::vector<std::string> CONFIG_FIELDS = { "Id", "Main_server_ip", "Main_server_port", "BD_ip", "BD_port", "BD_login", "BD_password", "Count_threads"};
	int __count_threads;
	std::shared_ptr<net::io_context> __ioc;
	std::shared_ptr<WorkerM> __session;
};