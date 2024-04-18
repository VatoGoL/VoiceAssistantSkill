#pragma once
#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <queue>
#include <boost/asio.hpp>
#include <boost/beast/ssl.hpp>
#include <exception>
#include "Modules/Worker/Worker.hpp"
#include <GlobalModules/Logger/Logger.hpp>
#include <GlobalModules/Configer/Configer.hpp>
#include "Modules/HTTPSServer/HTTPSServer.hpp"
#include "Modules/SSLSertificate/SSLSertificate.hpp"
#include <GlobalModules/ClientDB/ClientDB.hpp>
//using namespace std;

class MainServer:public std::enable_shared_from_this<MainServer> {
private:
	const int __TIME_UPDATE = 30;
	std::string __db_ip = "";
	std::string __db_login = "";
	std::string __db_password = "";
	int __port_db = 0;
	int __port_marusia_station = 0;
	int __port_mqtt = 0;
	int __port_worker_mqtt = 0;
	int __port_worker_mqtt_info = 0;
	int __port_worker_marusia = 0;
	std::vector<std::thread> __threads;

	std::shared_ptr<boost::asio::io_context> __io_ctx;
	std::shared_ptr<boost::asio::ssl::context> __ssl_ctx;
	std::shared_ptr<boost::asio::deadline_timer> __update_timer;
 	short __count_threads;

	std::map<std::string, std::string> __configuration;
	std::shared_ptr<Logger> __logger;
	std::shared_ptr<Configer> __configer;

	std::shared_ptr<std::map<std::string, std::vector<std::string>>> __sp_db_worker_marusia;
	std::shared_ptr<std::map<std::string, std::vector<std::string>>> __sp_db_worker_lu;
	std::shared_ptr<std::map<std::string, std::vector<std::string>>> __sp_db_marusia_station;
	std::shared_ptr<std::map<std::string, std::vector<std::string>>> __sp_db_lift_blocks;

	std::shared_ptr<https_server::Listener> __server_https;
	std::shared_ptr<worker_server::Server> __server_w_mqtt;
	std::shared_ptr<worker_server::Server> __server_w_marusia;
	//shared_ptr<net_repeater::Server> __server_mqtt_repeater;
	std::shared_ptr<ClientDB> __client_db;

	std::queue<std::string> __table_name;
	std::queue<std::vector<std::string>> __table_fields;
	std::queue<std::string> __table_conditions;

	void __loadDataBase();
	void __startServers(std::map<std::string, std::map<std::string, std::vector<std::string>>> data);
	void __updateData(std::map<std::string, std::map<std::string, std::vector<std::string>>> &data);
	void __updateDataCallback(std::map<std::string, std::map<std::string, std::vector<std::string>>> data);
	void __updateTimerCallback(const boost::system::error_code& error);
	
public:
	enum PROCESS_CODE {
		SUCCESSFUL = 0,
		CONFIG_FILE_NOT_OPEN,
		CONFIG_DATA_NOT_FULL
	};

	MainServer();
	~MainServer();

	PROCESS_CODE init(std::string path_to_config_file);
	void start();
	void stop();
};