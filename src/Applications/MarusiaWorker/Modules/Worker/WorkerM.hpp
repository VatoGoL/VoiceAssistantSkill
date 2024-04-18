#pragma once

#include <GlobalModules/Logger/Logger.hpp>
#include <GlobalModules/Configer/Configer.hpp>
#include <GlobalModules/JSONFormatter/JSONFormatter.hpp>
#include <GlobalModules/ClientDB/ClientDB.hpp>
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


class WorkerM : public std::enable_shared_from_this<WorkerM>
{
public:
	typedef std::function<void(std::map<std::string, std::map<std::string, std::vector<std::string>>> )> _callback_t;
	_callback_t callback;

	void __сallback(std::map<std::string, std::map<std::string, std::vector<std::string>>> data);

	WorkerM(std::map<std::string, std::string> conf_info, net::io_context& ioc, std::shared_ptr<Logger> lg);
	~WorkerM();
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
	std::string __port_ms;
	std::string __worker_id;
	std::string __ip_db;
	std::string __port_db;
	std::string __db_log;
	std::string __db_pas;

	std::map<std::string, std::map<std::string, std::vector<std::string>>> __db_info;
	/*vector<string> __marussiaStationFields = {"ApplicationId", "ComplexId", "HouseNum"};
	vector<string> __houseFields = { "TopFloor", "BottomFloor", "NullFloor", "HouseNum", "ComplexId" };
	vector<string> __staticPhrasesFields = { "ComplexId", "HouseNumber", "KeyWords", "Response" };*/


	static const int BUF_RECIVE_SIZE = 2048;
	std::string __buf_send;
	char* __buf_recive;
	std::queue<boost::json::value> __buf_json_recive;
	boost::json::value __buf_json_response;
	boost::json::stream_parser __parser;
	std::queue<std::string> __sender;
	std::shared_ptr<Logger> __log;
	std::shared_ptr<tcp::endpoint> __end_point;
	bool __flag_disconnect = false;
	bool __flag_end_send = false;
	std::string __response_command;
	boost::json::value __json_string;

	std::vector<std::string> __key_roots = { "перв", "втор", "трет", "чет", "пят", "шест", "седьм", "восьм", "девят","дцат", "сорок", "десят",  "девян", "сот","сто","минус", "нулев"};
	std::map<std::string, int> __num_roots= {
		{"минус третий", -3}, {"минус второй", -2}, {"минус первый", -1}, {"нулевой", 0},
		{"первый", 1}, {"второй", 2}, {"третий", 3}, {"четвертый", 4}, {"пятый", 5}, {"шестой", 6}, {"седьмой", 7}, {"восьмой", 8},{"девятый", 9}, {"десятый", 10},
		{"одиннадцатый", 11}, {"двенадцатый", 12}, {"тринадцатый", 13}, {"четырнадцатый", 14}, {"пятнадцатый", 15}, {"шестнадцатый", 16}, {"семнадцатый", 17}, {"восемнадцатый", 18},
		{"девятнадцатый", 19}, {"двадцатый", 20}, {"двадцать первый", 21}, {"двадцать второй", 22}, {"двадцать третий", 23}, {"двадцать четвертый", 24}, {"двадцать пятый", 25},
		{"двадцать шестой", 26}, {"двадцать седьмой", 27}, {"двадцать восьмой", 28}, {"двадцать девятый", 29}, {"тридцатый", 30}
	};
	std::vector<std::string> __mqtt_keys = { "этаж","подъем","спуск","подними","опусти" };

	std::map<std::string, std::string> __config_info;

	void __connectToMS();
	void __sendConnect(const boost::system::error_code& eC);
	void __reciveConnect(const boost::system::error_code& eC, size_t bytes_send);
	void __reciveCommand(const boost::system::error_code& eC, size_t bytes_recive);
	

	void __sendResponse(const boost::system::error_code& eC, size_t bytes_recive);
	void __reciveAnswer(const boost::system::error_code& eC, size_t bytes_send);
	void __connectToBd();
	void __resetTimer();

	void __analizeRequest();
	boost::json::object __getRespToMS(std::string respText);


	enum __CHECK_STATUS
	{
		SUCCESS = 1,
		FAIL
	};

	__CHECK_STATUS __checkSend(const size_t& count_send_byte, size_t& temp_send_byte, __handler_t&& handler);
	__CHECK_STATUS __checkJson(const size_t& countReciveByte, __handler_t&& handler);

};
//-------------------------------------------------------------//