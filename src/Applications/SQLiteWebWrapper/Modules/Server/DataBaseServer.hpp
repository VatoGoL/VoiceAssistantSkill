#pragma once

#define BOOST_BIND_GLOBAL_PLACEHOLDERS

#include "/media/vato/6c438f7c-3d6e-47b1-8c02-ca9aa1dc9ba2/vato/Vato/Diplom/SQLiteLib/SQLiteAPI/sqlite3.h"
#include <GlobalModules/JSONFormatter/JSONFormatter.hpp>
#include <GlobalModules/Logger/Logger.hpp>
#include <GlobalModules/Configer/Configer.hpp>
#include <boost/lambda2.hpp>
#include <boost/asio.hpp>
#include <boost/json.hpp>
#include <boost/config.hpp>
#include <boost/beast.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <typeinfo>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <locale.h>
#include <ctime>
#include <map>
#include <list>
#include <queue>
#include <boost/bind.hpp>

#define DB_WAY "./SmartLiftBase.db"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;


class DataBase : public std::enable_shared_from_this<DataBase>
{
private:

	const std::string __name = "Data_base";
	std::shared_ptr<tcp::endpoint>__end__point;
	std::shared_ptr<tcp::socket>__socket;

	static const int BUF_SIZE = 2048;
	std::string __buf_send;
	char* __buf_recieve;
	std::queue<boost::json::value> __buf_json_recieve;
	boost::json::stream_parser __parser;
	sqlite3* __dB;
	std::string __query;
	std::string __error_what;
	std::map<std::string, std::vector<std::string>> __answer;
	bool __flag_wrong_connect = false;
	bool __flag_send = false;
	bool __flag_exit = false;
	boost::json::value __json_string;

	void __reqAutentification();
	void __resAutentification(const boost::system::error_code& eC, size_t bytes_send);
	void __connectAnalize(const boost::system::error_code& eC, size_t bytes_recieve);
	
	void __waitCommand(const boost::system::error_code& eC, size_t bytes_recieve);
	void __sendCommand(const boost::system::error_code& eC, size_t bytes_send);

	std::string __checkCommand(char* __bufRecieve, size_t bytes_recieve);
	void __makePing();
	void __makeDisconnect();
	void __makeQuery();
	void __makeError();
	void __checkConnect(std::string login, std::string password);
	static int __connection(void* answer, int argc, char** argv, char** az_col_name);

public:
	DataBase(tcp::socket sock);
	void start();
	void stop();
	~DataBase();
	
	bool getFlagExit();
};

class Server : public std::enable_shared_from_this<Server>
{
public:
	enum PROCESS_CODE {
		SUCCESSFUL = 0,
		UNKNOWN_ERROR,
		CONFIG_DATA_NOT_FULL,
		CONFIG_DATA_NOT_CORRECT
	};

	Server(std::shared_ptr<net::io_context> io_context);
	PROCESS_CODE init();
	void run();
	void stop();
	~Server();
private:
	std::shared_ptr<tcp::acceptor> __acceptor;
	std::shared_ptr<net::io_context> __ioc;
	std::shared_ptr<std::vector<std::shared_ptr<DataBase>>> __sessions;
	std::shared_ptr<net::deadline_timer> __timer;

	void __doAccept();
	void __resetTimer();
};