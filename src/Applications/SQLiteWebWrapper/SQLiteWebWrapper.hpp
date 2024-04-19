#pragma once

#include "Modules/Server/DataBaseServer.hpp"
////////#include <GlobalModules/JSONFormatter/JSONFormatter.hpp>
#include <GlobalModules/Logger/Logger.hpp>
#include <GlobalModules/Configer/Configer.hpp>


class ServerDataBase: public std::enable_shared_from_this<ServerDataBase>
{
private:
	std::shared_ptr<net::io_context> __ioc;
	short __count_threads;
	std::shared_ptr<Server> __server;
public:
	enum PROCESS_CODE {
		SUCCESSFUL = 0,
		UNKNOWN_ERROR,
		LOG_FILE_NOT_OPEN,
		CONFIG_FILE_NOT_OPEN,
		CONFIG_DATA_NOT_FULL,
		CONFIG_DATA_NOT_CORRECT
	};

	ServerDataBase() = default;
	~ServerDataBase();
	PROCESS_CODE init(std::string config_way = "./", std::string name_conf = Configer::CONFIG_FILE);
	void start();
	void stop();
};