#pragma once

#include <GlobalModules/JSONFormatter/JSONFormatter.hpp>
#include <boost/lambda2.hpp>
#include <boost/asio.hpp>
#include <boost/json.hpp>
#include <boost/config.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
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

class ClientDB : public enable_shared_from_this<ClientDB>
{
public:
	
	typedef std::function<void(std::map<std::string, std::map<std::string, std::vector<std::string>>>)> callback_t;

	ClientDB(std::string ip_dB, std::string port_dB, 
             std::string worker_log, std::string worker_pass, 
             std::string name_sender, std::shared_ptr<tcp::socket> socket, callback_t callback);
	~ClientDB();
	void start();
	void stop();
	void setQuerys(std::queue<std::string> queue_tables, std::queue<std::vector<std::string>> queue_fields, 
                   std::queue<std::string> queue_conditions );

	void setCallback(callback_t callback);
	std::map<std::string, std::map<std::string, std::vector<std::string>>> getRespData();

private:
	void __emptyCallback(std::map<std::string, std::map<std::string, std::vector<std::string>>> data);
	void __checkConnect(const boost::system::error_code& error_code);

	void __queryGenerator();
	void __sendCommand(const boost::system::error_code& error_code, size_t bytes_send);
	void __reciveCommand(const boost::system::error_code& error_code, size_t bytes_recive);
	void __commandAnalize(const boost::system::error_code& error_code);     

	callback_t __callback_f;
	typedef std::function<void(boost::system::error_code, std::size_t)> __handler_t;
	std::shared_ptr<tcp::endpoint> __end_point;
	std::shared_ptr<tcp::socket> __socket;
	static const int BUF_RECIVE_SIZE = 2048;
	std::queue<std::string> __queue_tables;
	std::queue<std::vector<std::string>> __queue_fields;
	std::queue<std::string> __queue_conditions;
	std::string __buf_queue_string;
	char* __buf_recive;
	boost::json::value __buf_json_recive;
	boost::json::stream_parser __parser;
	std::string __resp_error;
	std::map<std::string, std::map<std::string, std::vector<std::string>>> __resp_data;
	std::string __worker_login;
	std::string __worker_password;
	std::string __table_name_send;
	std::vector<std::string> __fields_name_send;
	bool __flag_connet;
	bool __flag_disconnect;
	bool __flag_conditions;
	std::string __name_sender;

	enum __CHECK_STATUS
	{
		SUCCESS = 1,
		FAIL
	};

	__CHECK_STATUS __reciveCheck(const size_t& count_recive_byte, __handler_t&& handler);
	__CHECK_STATUS __sendCheck(const size_t& count_send_byte, size_t& temp_send_byte, __handler_t&& handler);
};