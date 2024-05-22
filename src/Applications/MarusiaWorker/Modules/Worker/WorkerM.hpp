#pragma once

#define BOOST_BIND_GLOBAL_PLACEHOLDERS

#include <GlobalModules/Logger/Logger.hpp>
#include <GlobalModules/Configer/Configer.hpp>
#include <GlobalModules/JSONFormatter/JSONFormatter.hpp>
#include <GlobalModules/ClientDB/ClientDB.hpp>
#include <GlobalModules/SystemStat/SystemStat.hpp>
#include "../ScheduleManager/ScheduleManager.hpp"
#include <boost/lambda2.hpp>
#include <boost/asio.hpp>
#include <boost/json.hpp>
#include <boost/config.hpp>
//#include <boost/property_tree/ptree.hpp>
//#include <boost/property_tree/json_parser.hpp>
#include <boost/bind.hpp>
#include <boost/beast.hpp> 
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <locale.h>
#include <ctime>
#include <map>
#include <list>
#include <queue>
#include <iostream>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;


class WorkerM : public std::enable_shared_from_this<WorkerM>
{
public:
	enum PROCESS_CODE {
		SUCCESSFUL = 0,
		UNKNOWN_ERROR,
		CONFIG_DATA_NOT_FULL,
		CONFIG_DATA_NOT_CORRECT
	};

	typedef std::function<void(std::map<std::string, std::map<std::string, std::vector<std::string>>> )> _callback_t;
	_callback_t callback;

	

	WorkerM(net::io_context& ioc);
	~WorkerM();
	
	PROCESS_CODE init();

	void start();
	void stop();
	void setDbInfo(std::map <std::string, std::map<std::string, std::vector<std::string>>> data);

private:
	typedef std::function<void(boost::system::error_code, std::size_t)> __handler_t;
	std::string __name = "Marusia_Worker";
	std::shared_ptr<tcp::socket>__socket;
	std::shared_ptr<tcp::socket> __socket_dB;
	net::io_context& __ioc;
	std::shared_ptr<ClientDB> __db_client;
	std::shared_ptr<net::deadline_timer> __timer;

	std::string __id;
	std::string __ip_ms;
	int __port_ms;
	std::string __worker_id;
	std::string __ip_db;
	int __port_db;
	std::string __db_log;
	std::string __db_pas;
	ScheduleManager __schedule_manager;

	static const int BUF_RECIVE_SIZE = 2048;
	std::string __buf_send;
	char* __buf_recive;
	std::queue<boost::json::value> __buf_json_recive;
	boost::json::value __buf_json_response;
	boost::json::stream_parser __parser;
	std::queue<std::string> __sender;
	std::shared_ptr<tcp::endpoint> __end_point;
	bool __flag_disconnect = false;
	bool __flag_end_send = false;
	std::string __response_command;
	std::string __text_presentation;
	std::string __text_opportunities;
	boost::json::value __json_string;
	std::map<std::string, std::vector<std::string>> __table_direction_of_preparation;
	std::map<std::string, std::vector<std::string>> __table_interesting_fact;
	std::map<std::string, std::vector<std::string>> __table_static_phrases;
	std::map<std::string, std::vector<std::string>> __table_university_fact;
	std::map<std::string, std::vector<std::string>> __table_groups_name;
	std::map<std::string, std::vector<std::string>> __table_professors_name;
	std::map<std::string, std::vector<std::string>> __table_days_week;
	std::map<std::string, std::vector<std::string>> __table_directions;
	std::map<std::string, std::pair<std::string, size_t>> __active_dialog_sessions;	
	std::vector<std::pair<std::vector<std::string>, std::string >> __vectors_variants;	
	std::vector<std::pair<std::vector<std::string>, std::string >> __vectors_variants_professors;
	std::vector<std::pair<std::vector<std::string>, std::string >> __vectors_variants_groups;	
	std::vector<std::pair<std::vector<std::string>, std::string >> __vectors_days_week;
	std::vector<std::pair<std::vector<std::string>, std::string >> __vectors_directions;													

	static void __parseKeyWords(std::vector<std::pair<std::vector<std::string>, std::string >> &vectors_variants,
						 const std::map<std::string, std::vector<std::string>>& table,
						 const std::string& key_words_name_field, const std::string& response_name_field);
	void __connectToMS();
	void __sendConnect(const boost::system::error_code& eC);
	void __reciveConnect(const boost::system::error_code& eC, size_t bytes_send);
	void __reciveCommand(const boost::system::error_code& eC, size_t bytes_recive);
	void __сallback(std::map<std::string, std::map<std::string, std::vector<std::string>>> data);

	void __sendResponse(const boost::system::error_code& eC, size_t bytes_recive);
	void __reciveAnswer(const boost::system::error_code& eC, size_t bytes_send);
	void __connectToDB();
	void __resetTimer();

	static std::string __findVariant(const std::vector<std::pair<std::vector<std::string>, std::string >>& variants, const std::string& data);
	std::string __findSchedule(const std::string& app_id, const std::vector<std::pair<std::vector<std::string>, std::string >>& variants_target, const std::string& command, const bool& is_professors);
	std::string __findDataInTable(const std::map<std::string, std::vector<std::string>>& table, 
								  const std::string& target_field, const std::string& target_value, 
								  const std::string& data_field);
	void __analizeRequest();
	boost::json::object __getRespToMS(const std::string& response_text);
	void __responseTypeAnalize(const std::string &response_type, const std::string& app_ip, const std::string& app_id, const std::string& command);
	void __dialogSessionsStep(const std::string& app_ip, const std::string &app_id,const std::string& command);
	void __clearZombieSessions();
	boost::json::array __getActiveDialogSessions();
	enum __CHECK_STATUS
	{
		SUCCESS = 1,
		FAIL
	};

	__CHECK_STATUS __checkSend(const size_t& count_send_byte, size_t& temp_send_byte, __handler_t&& handler);
	__CHECK_STATUS __checkJson(const size_t& countReciveByte, __handler_t&& handler);

};
//-------------------------------------------------------------//