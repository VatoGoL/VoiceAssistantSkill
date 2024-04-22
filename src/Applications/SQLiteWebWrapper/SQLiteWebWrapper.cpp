#include "SQLiteWebWrapper.hpp"

ServerDataBase::PROCESS_CODE ServerDataBase::init(std::string config_way, std::string name_conf)
{
    try{
		Logger::init();
		Configer::init(config_way,name_conf);
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
    std::map<std::string, std::string>& configuration = Configer::getConfigInfo();
    try{
        __count_threads = std::stoi(configuration.at("Count_threads"));
    }catch(std::exception &e){
        Logger::writeLog("ServerDataBase","init",Logger::log_message_t::ERROR,e.what());
		return PROCESS_CODE::CONFIG_DATA_NOT_FULL;
    }
    
    __ioc = std::make_shared<boost::asio::io_context>(__count_threads);
    __server = std::make_shared<Server>(__ioc);
    switch(__server->init()){
        case Server::UNKNOWN_ERROR:
            return UNKNOWN_ERROR;
            break;
		case Server::CONFIG_DATA_NOT_FULL:
            return CONFIG_DATA_NOT_FULL;
            break;
		case Server::CONFIG_DATA_NOT_CORRECT:
            return CONFIG_DATA_NOT_CORRECT;
            break;
    }
    return SUCCESSFUL;
}
ServerDataBase::~ServerDataBase()
{
    stop();
}
void ServerDataBase::start()
{
    __server->run();

    std::vector<std::thread> v;
    v.reserve(__count_threads - 1);
    for (auto i = __count_threads - 1; i > 0; --i)
        v.emplace_back(
            [this]
            {
                __ioc->run();
            });
    __ioc->run();
}
void ServerDataBase::stop()
{
    __server->stop();
}