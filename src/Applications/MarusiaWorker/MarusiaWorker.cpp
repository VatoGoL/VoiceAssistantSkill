#include "MarusiaWorker.hpp"

MarusiaWorker::PROCESS_CODE MarusiaWorker::init(std::string way_to_conf_file, std::string name_conf_file)
{
 
    try{
		Logger::init();
		Configer::init("./",way_to_conf_file,name_conf_file);
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
    
	try {
		__count_threads = stoi(configuration.at("Count_threads"));
        if (__count_threads < 1) 
		{
			Logger::writeLog("MarusiaWorker","init",Logger::log_message_t::ERROR,"Port <= 0");
			return PROCESS_CODE::CONFIG_DATA_NOT_CORRECT;
		}
		__ioc = std::make_shared<boost::asio::io_context>(__count_threads);
        __session = std::make_shared<WorkerM>(*__ioc);
	}
	catch (std::exception& e) {
		Logger::writeLog("MarusiaWorker","init",Logger::log_message_t::ERROR,e.what());
		return PROCESS_CODE::CONFIG_DATA_NOT_FULL;
	}

    __ioc = std::make_shared<boost::asio::io_context>(__count_threads);
    __session = std::make_shared<WorkerM>(*__ioc);
	switch(__session->init()){
		case WorkerM::PROCESS_CODE::CONFIG_DATA_NOT_CORRECT:
			Logger::writeLog("MarusiaWorker", "init", Logger::log_message_t::ERROR, "Config file not correct");
			return PROCESS_CODE::CONFIG_DATA_NOT_CORRECT;
		break;
		case WorkerM::PROCESS_CODE::CONFIG_DATA_NOT_FULL:
			Logger::writeLog("MarusiaWorker", "init", Logger::log_message_t::ERROR, "Config file not full");
			return PROCESS_CODE::CONFIG_DATA_NOT_FULL;
		break;
		case WorkerM::PROCESS_CODE::UNKNOWN_ERROR:
			Logger::writeLog("MarusiaWorker", "init", Logger::log_message_t::ERROR, "Unknown error");
			return PROCESS_CODE::UNKNOWN_ERROR;
		break;
	}
	return PROCESS_CODE::SUCCESSFUL;
}
void MarusiaWorker::run()
{ 
    __session->start();
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
void MarusiaWorker::stop()
{
    __session->stop();
}
MarusiaWorker::~MarusiaWorker()
{
    this->stop();
}