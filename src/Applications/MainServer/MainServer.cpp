#include "MainServer.hpp"

using namespace boost::placeholders;
MainServer::MainServer() {}
MainServer::~MainServer() {
	stop();
}
MainServer::PROCESS_CODE MainServer::init(std::string path_to_config_file) {

	__logger = std::make_shared<Logger>("","./","MainServer");
	__configer = std::make_shared<Configer>(__logger, "./", path_to_config_file);
	__configer->readConfig();
	__ssl_ctx = std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12);

	std::map<std::string, std::string> configuration = __configer->getConfigInfo();
	try {
		if (configuration.size() == 0) {
			return CONFIG_FILE_NOT_OPEN;
		}
		__db_ip = configuration.at("DB_ip");
		__db_login = configuration.at("DB_login");
		__db_password = configuration.at("DB_password");
		__port_db = stoi(configuration.at("DB_port"));
		__port_marusia_station = stoi(configuration.at("Marusia_port"));
		__port_mqtt = stoi(configuration.at("MQTT_port"));
		__port_worker_mqtt = stoi(configuration.at("Worker_MQTT_port"));;
		__port_worker_mqtt_info = stoi(configuration.at("Worker_MQTT_info_port"));
		__port_worker_marusia = stoi(configuration.at("Worker_marusia_port"));
		__count_threads = stoi(configuration.at("Count_threads"));
		if(load_server_certificate(*__ssl_ctx, configuration.at("Path_to_ssl_sertificate"),configuration.at("Path_to_ssl_key")) == -1){
			throw std::invalid_argument("Lead sertificate error");
		}
		if (__port_marusia_station < 1 || __port_mqtt < 1 || __port_worker_mqtt < 1 ||
			__port_worker_mqtt_info < 1 || __port_worker_marusia < 1 || __count_threads < 1) 
		{
			//throw exception("Port <= 0");
			throw std::invalid_argument("Port <= 0");
		}
	}
	catch (std::invalid_argument& e) {
		std::cerr << e.what() << std::endl;
		return PROCESS_CODE::CONFIG_DATA_NOT_FULL;
	}
	
	__io_ctx = std::make_shared<boost::asio::io_context>(__count_threads);
	
	
	//__server_w_mqtt = std::make_shared<worker_server::Server>(__io_ctx, __port_worker_mqtt_info,worker_server::WORKER_MQTT_T);
	__server_w_marusia = std::make_shared<worker_server::Server>(__io_ctx, __port_worker_marusia, worker_server::WORKER_MARUSIA_T);
	__server_https = std::make_shared<https_server::Listener>(*__io_ctx, *__ssl_ctx,configuration.at("Path_to_ssl_sertificate"),configuration.at("Path_to_ssl_key"),__port_marusia_station, __server_w_marusia->getSessions());
	__client_db = std::make_shared<ClientDB>(__db_ip, std::to_string(__port_db), __db_login, __db_password, 
										"Main_server", std::make_shared<boost::asio::ip::tcp::socket>(*__io_ctx), 
										bind(&MainServer::__updateDataCallback, this,_1));
		
	__update_timer = std::make_shared<boost::asio::deadline_timer>(*__io_ctx);
	//__server_mqtt_repeater = make_shared<net_repeater::Server>(__io_ctx, __port_mqtt, __server_w_mqtt->getSessions());
	return PROCESS_CODE::SUCCESSFUL;
}
void MainServer::stop(){
	__server_w_marusia->stop();
}
void MainServer::start() {
	
	__loadDataBase();

	__threads.reserve(__count_threads - 1);
	for (auto i = __count_threads - 1; i > 0; --i)
		__threads.emplace_back(
			[this]
			{
				__io_ctx->run();
			});
	__io_ctx->run();
}
void MainServer::__startServers(std::map<std::string, std::map<std::string, std::vector<std::string>>> data) {
	std::cout << "Start servers" << std::endl;
	
	try {
		__updateData(data);
	}
	catch (std::exception& e) {
		std::cerr << "StartServers: " << e.what() << std::endl;
		return;
	}
	
	__client_db->setCallback(bind(&MainServer::__updateDataCallback, this, _1));
	__update_timer->expires_from_now(boost::posix_time::seconds(__TIME_UPDATE));
	__update_timer->async_wait(boost::bind(&MainServer::__updateTimerCallback, this, _1));
	
	__server_w_marusia->start(std::make_shared<std::shared_ptr<std::map<std::string, std::vector<std::string>>>>(__sp_db_worker_marusia));
	__server_https->start(std::make_shared<std::shared_ptr<std::map<std::string, std::vector<std::string>>>>(__sp_db_marusia_station));
}
void MainServer::__loadDataBase() {
	__client_db->setCallback(bind(&MainServer::__startServers, this, _1));
	
	__table_name.push("WorkerMarussia");
	__table_fields.push({"Id"});
	__table_conditions.push("");
	
	__table_name.push("WorkerLU");
	__table_fields.push({ "Id", "Address", "Port"});
	__table_conditions.push("");

	__table_name.push("MarussiaStation");
	__table_fields.push({ "ApplicationId", "WorkerId", "SecondWorkerId", "LiftBlockId"});
	__table_conditions.push("");

	__table_name.push("LiftBlocks");
	__table_fields.push({ "Id", "WorkerLuId", "SecondWorkerLuId", "Descriptor"});
	__table_conditions.push("");

	__client_db->setQuerys(__table_name, __table_fields, __table_conditions);
	__client_db->start();
}
void MainServer::__updateData(std::map<std::string, std::map<std::string, std::vector<std::string>>> &data) {
	std::shared_ptr<std::map<std::string, std::vector<std::string>>> temp_sp_db_worker_marusia = std::make_shared<std::map<std::string, std::vector<std::string>>>(data.at("WorkerMarussia"));
	std::shared_ptr<std::map<std::string, std::vector<std::string>>> temp_sp_db_worker_lu = std::make_shared<std::map<std::string, std::vector<std::string>>>(data.at("WorkerLU"));
	std::shared_ptr<std::map<std::string, std::vector<std::string>>> temp_sp_db_marusia_station = std::make_shared<std::map<std::string, std::vector<std::string>>>(data.at("MarussiaStation"));
	std::shared_ptr<std::map<std::string, std::vector<std::string>>> temp_sp_db_lift_blocks = std::make_shared<std::map<std::string, std::vector<std::string>>>(data.at("LiftBlocks"));
	
	__sp_db_worker_marusia = temp_sp_db_worker_marusia;
	__sp_db_marusia_station = temp_sp_db_marusia_station;
	__sp_db_lift_blocks = temp_sp_db_lift_blocks;
}
void MainServer::__updateDataCallback(std::map<std::string, std::map<std::string, std::vector<std::string>>> data){
	try {
		__updateData(data);
	}
	catch (std::exception& e) {
		std::cerr << "__updateDataCallback: " << e.what();
		return;
	}
}
void MainServer::__updateTimerCallback(const boost::system::error_code& error){
	__client_db->setQuerys(__table_name, __table_fields, __table_conditions);
	__client_db->start();
	__update_timer->expires_from_now(boost::posix_time::seconds(__TIME_UPDATE));
	__update_timer->async_wait(boost::bind(&MainServer::__updateTimerCallback, this,_1));
}