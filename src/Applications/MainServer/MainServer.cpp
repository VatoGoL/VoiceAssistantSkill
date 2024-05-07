#include "MainServer.hpp"

using namespace boost::placeholders;
MainServer::MainServer() {}
MainServer::~MainServer() {
	stop();
}
MainServer::PROCESS_CODE MainServer::init(std::string path_to_config_file) {

	try{
		Logger::init();
		Configer::init("./",path_to_config_file);
	}catch(Logger::error_logger_t &e){
		switch(e){
			case Logger::error_logger_t::FILE_NOT_OPEN:
				return LOG_FILE_NOT_OPEN;
			break;
			default:
				return UNKNOWN_ERROR;
			break;
		}
	}catch(Configer::error_configer_t &e){
		switch(e){
			case Configer::error_configer_t::FILE_NOT_OPEN:
				return CONFIG_FILE_NOT_OPEN;
			break;
			default:
				return UNKNOWN_ERROR;
			break;
		}
	}
	
	Configer::readConfig();
	__ssl_ctx = std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12);

	std::map<std::string, std::string>& configuration = Configer::getConfigInfo();
	try {
		__db_ip = configuration.at("DB_ip");
		__db_login = configuration.at("DB_login");
		__db_password = configuration.at("DB_password");
		__port_db = stoi(configuration.at("DB_port"));
		__port_marusia_station = stoi(configuration.at("Marusia_port"));
		__port_worker_marusia = stoi(configuration.at("Worker_marusia_port"));
		__count_threads = stoi(configuration.at("Count_threads"));
		if(load_server_certificate(*__ssl_ctx, configuration.at("Path_to_ssl_sertificate"),configuration.at("Path_to_ssl_key")) == -1){
			Logger::writeLog("MainServer","init",Logger::log_message_t::ERROR,"Load sertificate error");
			return PROCESS_CODE::CONFIG_DATA_NOT_CORRECT; 
		}
		if (__port_marusia_station < 1 || __port_worker_marusia < 1 || __count_threads < 1) 
		{
			Logger::writeLog("MainServer","init",Logger::log_message_t::ERROR,"Port <= 0");
			return PROCESS_CODE::CONFIG_DATA_NOT_CORRECT;
		}
	}
	catch (std::exception& e) {
		Logger::writeLog("MainServer","init",Logger::log_message_t::ERROR,e.what());
		return PROCESS_CODE::CONFIG_DATA_NOT_FULL;
	}
	
	__io_ctx = std::make_shared<boost::asio::io_context>(__count_threads);
	
	__server_w_marusia = std::make_shared<worker_server::Server>(__io_ctx, __port_worker_marusia, worker_server::WORKER_MARUSIA_T);
	__server_https = std::make_shared<https_server::Listener>(*__io_ctx, *__ssl_ctx,configuration.at("Path_to_ssl_sertificate"),configuration.at("Path_to_ssl_key"),__port_marusia_station, __server_w_marusia->getSessions());
	__client_db = std::make_shared<ClientDB>(__db_ip, __port_db, __db_login, __db_password, 
										"Main_server", std::make_shared<boost::asio::ip::tcp::socket>(*__io_ctx), 
										bind(&MainServer::__updateDataCallback, this,_1));
		
	__update_timer = std::make_shared<boost::asio::deadline_timer>(*__io_ctx);
	
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
	Logger::writeLog("MainServer", "__start_Servers",Logger::log_message_t::EVENT, "Start servers");
	
	try {
		__updateData(data);
	}
	catch (std::exception& e) {
		Logger::writeLog("MainServer", "__start_Servers",Logger::log_message_t::ERROR, e.what());
		return;
	}
	
	//__client_db->setCallback(boost::bind(&MainServer::__updateDataCallback, this, _1));
	//__update_timer->expires_from_now(boost::posix_time::seconds(__TIME_UPDATE));
	//__update_timer->async_wait(boost::bind(&MainServer::__updateTimerCallback, this, _1));
	
	__server_w_marusia->start();
	__server_https->start();
}
void MainServer::__loadDataBase() {
	__client_db->setCallback(bind(&MainServer::__startServers, this, _1));
	
	/*__table_name.push("InterestingFact");
	__table_fields.push({"fact"});
	__table_conditions.push("");
	
	__table_name.push("UniversityFact");
	__table_fields.push({"fact"});
	__table_conditions.push("");

	__table_name.push("DirectionOfPreparation");
	__table_fields.push({ "direction", "number_of_budget_positions", "minimum_score"});
	__table_conditions.push("");*/

	//__client_db->setQuerys(__table_name, __table_fields, __table_conditions);
	//__client_db->start();
}
void MainServer::__updateData(const std::map<std::string, std::map<std::string, std::vector<std::string>>> &data) {
	//std::shared_ptr<std::map<std::string, std::vector<std::string>>> temp_sp_db_interesting_fact = std::make_shared<std::map<std::string, std::vector<std::string>>>(data.at("InterestingFact"));
	//std::shared_ptr<std::map<std::string, std::vector<std::string>>> temp_sp_db_university_fact = std::make_shared<std::map<std::string, std::vector<std::string>>>(data.at("UniversityFact"));
	//std::shared_ptr<std::map<std::string, std::vector<std::string>>> temp_sp_db_direction_of_preparation = std::make_shared<std::map<std::string, std::vector<std::string>>>(data.at("DirectionOfPreparation"));
	
	/*__sp_db_interesting_fact = temp_sp_db_interesting_fact;
	__sp_db_university_fact = temp_sp_db_university_fact;
	__sp_db_direction_of_preparation = temp_sp_db_direction_of_preparation;*/
}
void MainServer::__updateDataCallback(std::map<std::string, std::map<std::string, std::vector<std::string>>> data){
	try {
		__updateData(data);
	}
	catch (std::exception& e) {
		Logger::writeLog("MainServer", "__updateDataCallback",Logger::log_message_t::ERROR, e.what());
		return;
	}
}
void MainServer::__updateTimerCallback(const boost::system::error_code& error){
	__client_db->setQuerys(__table_name, __table_fields, __table_conditions);
	__client_db->start();
	__update_timer->expires_from_now(boost::posix_time::seconds(__TIME_UPDATE));
	__update_timer->async_wait(boost::bind(&MainServer::__updateTimerCallback, this,_1));
}