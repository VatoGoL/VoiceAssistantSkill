#include "DataBaseServer.hpp"

void DataBase::__reqAutentification()
{
    __buf_send = "";
    __socket->async_receive(net::buffer(__buf_recieve, BUF_SIZE), boost::bind(&DataBase::__connectAnalize, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
}
void DataBase::__resAutentification(const boost::system::error_code& eC, size_t bytes_send)
{
    if (eC)
    {
        Logger::writeLog("DataBase", "__resAutentification", Logger::log_message_t::ERROR, "Failed to read response" + eC.message());
        #ifndef UNIX
            sleep(2);
        #else
            Sleep(2000);
        #endif
        __socket->async_send(net::buffer(__buf_send, __buf_send.size()), boost::bind(&DataBase::__resAutentification, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
        return;
    }
    static size_t temp_send = 0;
    temp_send += bytes_send;
    if (temp_send != __buf_send.size())
    {
        Logger::writeLog("DataBase", "__resAutentification", Logger::log_message_t::ERROR, "JSON not full");
        __socket->async_send(net::buffer(__buf_send.c_str() + temp_send, __buf_send.size() - temp_send), boost::bind(&DataBase::__resAutentification, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
        return;
    }
    temp_send = 0;
    __buf_send.clear();

    if (__flag_wrong_connect)
    {
        this->stop();
        __flag_wrong_connect = false;
    }
    if (__flag_send != true)
    {
        Logger::writeLog("DataBase", "__resAutentification", Logger::log_message_t::ERROR, "not full connection");
        std::cerr << "not full connection" << std::endl;
    }
    else
    {
        __flag_send = false;
        __socket->async_receive(net::buffer(__buf_recieve, BUF_SIZE), boost::bind(&DataBase::__waitCommand, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
    }
}
void DataBase::__connectAnalize(const boost::system::error_code& eC, size_t bytes_recieve)
{
    if (eC)
    {
        Logger::writeLog("DataBase", "__connectAnalize", Logger::log_message_t::ERROR, "failed read request" + eC.message());
        #ifndef UNIX
            sleep(1);
        #else
            Sleep(1000);
        #endif
        __socket->async_receive(net::buffer(__buf_recieve, BUF_SIZE), boost::bind(&DataBase::__connectAnalize, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
        return;
    }
    size_t count_byte, count_byte_write = 0;
    for (; count_byte_write < bytes_recieve;)
    {
        try
        {
            count_byte = __parser.write_some(__buf_recieve + count_byte_write);
        }
        catch (std::exception& e)
        {
            std::cerr << e.what() << std::endl;
        }
        if (!__parser.done())
        {
            std::fill_n(__buf_recieve, BUF_SIZE, 0);
            Logger::writeLog("DataBase", "__connectAnalize", Logger::log_message_t::ERROR, "json not full");
            __socket->async_receive(net::buffer(__buf_recieve, BUF_SIZE), boost::bind(&DataBase::__connectAnalize, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
            return;
        }
        try
        {
            __buf_json_recieve.push(__parser.release());
            __parser.reset();
            count_byte_write += count_byte;
        }
        catch (std::exception& e)
        {
            __parser.reset();
            __buf_json_recieve = {};
            Logger::writeLog("DataBase", "__connectAnalize", Logger::log_message_t::ERROR, std::string("Json read ") + e.what());
            __socket->async_receive(net::buffer(__buf_recieve, BUF_SIZE), boost::bind(&DataBase::__connectAnalize, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
            return;
        }
    }
    std::fill_n(__buf_recieve, BUF_SIZE, 0);
    while(__buf_json_recieve.size() != 0)
    {
        try
        {
            __json_string = __buf_json_recieve.front();
            __buf_json_recieve.pop();
            std::string login = boost::json::serialize(__json_string.at("request").at("login"));
            std::string password = boost::json::serialize(__json_string.at("request").at("password"));
            __checkConnect(login, password);
            if (__buf_json_recieve.size() == 0)
            {
                __flag_send = true;
            }
            __socket->async_send(net::buffer(__buf_send, __buf_send.size()), boost::bind(&DataBase::__resAutentification, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
        }
        catch (std::exception& e)
        {
            std::cerr << e.what() << std::endl;
            __socket->async_receive(net::buffer(__buf_recieve, BUF_SIZE), boost::bind(&DataBase::__connectAnalize, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
        }
    }
}
void DataBase::__waitCommand(const boost::system::error_code& eC, size_t bytes_recieve)
{
    if (eC) {
        Logger::writeLog("DataBase", "__waitCommand", Logger::log_message_t::ERROR, "Error_failed_to_read_request" + eC.message());
        #ifndef UNIX
            sleep(1);
        #else
            Sleep(1000);
        #endif
        __socket->async_receive(boost::asio::buffer(__buf_recieve, BUF_SIZE),
            boost::bind(&DataBase::__waitCommand, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
        return;
    }

    size_t count_byte, count_byte_write = 0;
    for (; count_byte_write < bytes_recieve;)
    {
        try
        {
            count_byte = __parser.write_some(__buf_recieve + count_byte_write);
        }
        catch (std::exception& e)
        {
            std::cerr << e.what() << std::endl;
        }
        if (!__parser.done())
        {
            std::fill_n(__buf_recieve, BUF_SIZE, 0);
            Logger::writeLog("DataBase", "__waitCommand", Logger::log_message_t::ERROR, std::string("Json not full"));
            __socket->async_receive(net::buffer(__buf_recieve, BUF_SIZE), boost::bind(&DataBase::__connectAnalize, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
            return;
        }
        try
        {
            __buf_json_recieve.push(__parser.release());
            __parser.reset();
            count_byte_write += count_byte;
        }
        catch (std::exception& e)
        {
            __parser.reset();
            __buf_json_recieve = {};
            Logger::writeLog("DataBase", "__waitCommand", Logger::log_message_t::ERROR, std::string("Json read") + e.what());
            __socket->async_receive(net::buffer(__buf_recieve, BUF_SIZE), boost::bind(&DataBase::__connectAnalize, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
            return;
        }
    }
    std::fill_n(__buf_recieve, BUF_SIZE, 0);
    while(__buf_json_recieve.size() != 0)
    {
        __json_string = __buf_json_recieve.front();
        __buf_json_recieve.pop();
        std::string command = __checkCommand(__buf_recieve, bytes_recieve);
        if (command == "ping")
        {
            __makePing();
        }
        else if (command == "disconnect")
        {
            __makeDisconnect();
        }
        else if (command == "db_query")
        {
            __makeQuery();
        }
        else if (command == "error")
        {
            
            __makeError();
        }
        else if (command == "connect")
        {
            std::cout << "connect" << std::endl;
            __connectAnalize(eC, bytes_recieve);
        }
        if (__buf_json_recieve.size() == 0)
        {
            __flag_send = true;
        }

        __socket->async_send(net::buffer(__buf_send, __buf_send.size()), boost::bind(&DataBase::__sendCommand, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
    }
}
void DataBase::__sendCommand(const boost::system::error_code& eC, size_t bytes_send)
{
    if (eC)
    {
        Logger::writeLog("DataBase", "__sendCommand", Logger::log_message_t::ERROR, "error send response command " + eC.message());
        #ifndef UNIX
            sleep(1);
        #else
            Sleep(1000);
        #endif
        __socket->async_send(net::buffer(__buf_send, __buf_send.size()), boost::bind(&DataBase::__sendCommand, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
        return;
    }
    static size_t temp_send = 0;
    temp_send += bytes_send;
    if (temp_send != __buf_send.size())
    {
        Logger::writeLog("DataBase", "__sendCommand", Logger::log_message_t::EVENT, "Not full json");
        __socket->async_send(net::buffer(__buf_send.c_str() + temp_send, __buf_send.size() - temp_send), boost::bind(&DataBase::__sendCommand, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
        return;
    }
    temp_send = 0;
    __buf_send.clear();
    if (__flag_wrong_connect)
    {
        __flag_wrong_connect = false;
        this->stop();
    }
    else
    {
        if(__flag_send)
        {
            __socket->async_receive(net::buffer(__buf_recieve, BUF_SIZE), boost::bind(&DataBase::__waitCommand, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
        }
        else
        {
            Logger::writeLog("DataBase", "__sendCommand", Logger::log_message_t::EVENT, "Not full send");
        }
    }
}
std::string DataBase::__checkCommand(char* __bufRecieve, size_t bytes_recieve)
{
    if (__json_string.at("target") == "ping")
    {
        return "ping";
    }
    else if (__json_string.at("target") == "disconnect")
    {
        return "disconnect";
    }
    else if (__json_string.at("target") == "db_query")
    {
        return "db_query";
    }
    else if (__json_string.at("target") == "connect")
    {
        return "connect";
    }
    else
    {
        Logger::writeLog("DataBase", "__checkCommand", Logger::log_message_t::EVENT, "No found command");
        __error_what.clear();
        __error_what = "non-existent command";
        return "error";//no command
    }
}
void DataBase::__makePing()
{
    __buf_send.clear();
    __buf_send = boost::json::serialize(json_formatter::database::response::ping(__name));
}
void DataBase::__makeDisconnect()
{
    __buf_send.clear();
    __buf_send = boost::json::serialize(json_formatter::database::response::disconnect(__name));
    __flag_wrong_connect = true;
}
void DataBase::__makeQuery()
{
    boost::json::array jsonFields = __json_string.at("request").at("fields").as_array();
    std::vector<std::string> fields;
    for (size_t i = 0; i < jsonFields.size(); i++)
    {
        fields.emplace_back(jsonFields[i].as_string());
    }
    __query.clear();

    __answer.clear();
    __query = __json_string.at("request").at("query").as_string();
    std::cout << "query " << __query << std::endl;

    int exit = sqlite3_open(DB_WAY, &__dB);
    if (exit != SQLITE_OK)
    {
        std::cerr << "NO DB FILE" << std::endl;
        __makeDisconnect();
    }
    else
    {
        char* error_msg = 0;
        int flag = sqlite3_exec(__dB, __query.c_str(), __connection, (void*)&__answer, &error_msg);
        if (flag != SQLITE_OK)
        {
            fprintf(stderr, "SQL error: %s\n", error_msg);
            sqlite3_free(error_msg);
            std::cout << "not exist query" << std::endl;
            __makeDisconnect();
            sqlite3_close(__dB);
        }
        else
        {
            sqlite3_close(__dB);

            std::map<std::string, std::vector<std::string>> response;
            std::map<std::string, std::vector<std::string>>::iterator it = __answer.begin();

            for (size_t i = 0; i < fields.size(); i++)
            {
                std::vector<std::string> buf = __answer.at(fields[i]);
                response[fields[i]] = buf;
            }

            __buf_send.clear();
            __buf_send = boost::json::serialize(json_formatter::database::response::query(__name, json_formatter::database::QUERY_METHOD::SELECT, response));
            std::cout << __buf_send << std::endl;
        }
    }
}
void DataBase::__makeError()
{
    __buf_send.clear();
    //__bufSend = boost::json::serialize(json_formatter::database::) //íåò êîìàíäû
}
void DataBase::__checkConnect(std::string login, std::string password)
{
    int exit = sqlite3_open(DB_WAY, &__dB);
    if (exit != SQLITE_OK)
    {
        std::cerr << "NO DB FILE" << std::endl;
        __buf_send = boost::json::serialize(json_formatter::database::response::connect(__name, json_formatter::ERROR_CODE::CONNECT, "No dataBase"));
        __flag_wrong_connect = true;
    }
    else
    {
        __query.clear();
        __query = "SELECT * FROM Accounts";
        __buf_send.clear();
        __answer.clear();
        char* error_msg = 0;
        std::cout << "query " << __query << std::endl;
        int flag = sqlite3_exec(__dB, __query.c_str(), __connection, (void*)&__answer, &error_msg);
        if (flag != SQLITE_OK)
        {
            fprintf(stderr, "SQL error: %s\n", error_msg);
            sqlite3_free(error_msg);
            std::cout << "not ok exec" << std::endl;
            __buf_send = boost::json::serialize(json_formatter::database::response::connect(__name, json_formatter::ERROR_CODE::CONNECT, "User not found"));
            __flag_wrong_connect = true;
            sqlite3_close(__dB);
        }
        else
        {
            sqlite3_close(__dB);
            try
            {
                std::vector<std::string> __login = __answer.at("login");
                std::vector<std::string> __password = __answer.at("password");
                int num_pus = -1;
                for (size_t i = 0; i < __login.size(); i++)
                {
                    std::string buf_log = "\"" + __login[i] + "\"";
                    std::cout << buf_log << " " << login << std::endl;
                    if (buf_log == (std::string)login)
                    {
                        num_pus = i;
                        break;
                    }
                }
                std::string buf_pas = "\"" + __password[num_pus] + "\"";
                std::cout << buf_pas << " " << password << std::endl;
                if (buf_pas == password)
                {
                    __buf_send = boost::json::serialize(json_formatter::database::response::connect(__name));
                }
                else
                {
                    std::cout << "flag" << std::endl;
                    __buf_send = boost::json::serialize(json_formatter::database::response::connect(__name, json_formatter::ERROR_CODE::CONNECT, "Password mismatch"));
                    __flag_wrong_connect = true;
                }
            }
            catch (std::exception& e)
            {
                std::cerr <<"__checkConnect: " << e.what() << std::endl;
            }
        }
        sqlite3_close(__dB);
    }
}
int DataBase::__connection(void* answer, int argc, char** argv, char** az_col_name)
{
    try
    {
        std::map<std::string, std::vector<std::string>>* buf_map = (std::map<std::string, std::vector<std::string>>*)answer;
        for (size_t i = 0; i < argc; i++)
        {
            std::string buf_answer = argv[i];
            std::string col_name = az_col_name[i];
            std::vector<std::string> buf_vec;
            try
            {
                buf_vec = buf_map->at(col_name);
                buf_vec.push_back(buf_answer);
            }
            catch (std::exception e)
            {
                buf_vec.push_back(buf_answer);
            }
            //buf_vec.push_back(buf_answer);
            (*buf_map)[col_name] = buf_vec;
        }
        return 0;
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
    return 1;
}
DataBase::DataBase(tcp::socket sock)
{
    __socket = std::make_shared<tcp::socket>(std::move(sock));
    __buf_recieve = new char[BUF_SIZE + 1];

    __buf_send = "";
    __buf_json_recieve = {};
    __parser.reset();
}
void DataBase::start()
{
    int checkDb = sqlite3_open(DB_WAY, &__dB);
    if (checkDb)
    {
        Logger::writeLog("DataBase", "start", Logger::log_message_t::ERROR, "DB not open");
        sqlite3_close(__dB);
        this->stop();
        return;
    }
    else
    {
        Logger::writeLog("DataBase", "start", Logger::log_message_t::EVENT, "DB open");
        sqlite3_close(__dB);
    }
    __reqAutentification();
}
void DataBase::stop()
{
    try
    {
        if (__socket->is_open())
        {
            __socket->close();
        }
        Logger::writeLog("DataBase", "stop", Logger::log_message_t::EVENT, "END connect");
        __flag_exit = true;
    }
    catch(std::exception &e)
    {
        Logger::writeLog("DataBase", "stop", Logger::log_message_t::ERROR, e.what());
    }
}
DataBase::~DataBase()
{
    this->stop();
    delete[] __buf_recieve;
}

bool DataBase::getFlagExit()
{
    return __flag_exit;
}

/*----------------------------------------------------------*/

Server::Server(std::shared_ptr<net::io_context> io_context): __ioc(io_context){}
Server::PROCESS_CODE Server::init()
{
    short port;
    try{
        port = std::stoi(Configer::getConfigInfo().at("Port"));
    }catch(std::exception &e){
        Logger::writeLog("Server","init",Logger::log_message_t::ERROR,e.what());
		return PROCESS_CODE::CONFIG_DATA_NOT_FULL;
    }
    if(port < 1){
        Logger::writeLog("Server","init",Logger::log_message_t::ERROR,"Port <= 0");
        return PROCESS_CODE::CONFIG_DATA_NOT_CORRECT;
    }

    __sessions = std::make_shared<std::vector<std::shared_ptr<DataBase>>>();
    __acceptor = std::make_shared<boost::asio::ip::tcp::acceptor>(*__ioc, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port));
    __timer = std::make_shared<net::deadline_timer>(*__ioc);
    return SUCCESSFUL;
}
void Server::run()
{
    __timer->expires_from_now(boost::posix_time::hours(24));
    __timer->async_wait(boost::bind(&Server::__resetTimer, this));
    __doAccept();
}
void Server::stop()
{
    for (auto i = __sessions->begin(), end = __sessions->end(); i != end; i++) {
        (*i)->stop();
    }
    __sessions->clear();
}
Server::~Server()
{
    this->stop();
}
void Server::__doAccept()
{
    
    __acceptor->async_accept([this](boost::system::error_code error, tcp::socket socket)
        {
            if (error) {
                std::cerr << error.message() << std::endl;
                __doAccept();
            }
            __sessions->push_back(std::make_shared<DataBase>(std::move(socket)));
            __sessions->back()->start();
            __doAccept();
        }
    );
}

void Server::__resetTimer()
{
    for (auto i = __sessions->begin(), end = __sessions->end(); i != end; i++)
    {
        if ((*i)->getFlagExit() == true)
        {
            __sessions->erase(i);
        }
    }
    __timer->expires_from_now(boost::posix_time::seconds(5));
    __timer->async_wait(boost::bind(&Server::__resetTimer, shared_from_this()));
}