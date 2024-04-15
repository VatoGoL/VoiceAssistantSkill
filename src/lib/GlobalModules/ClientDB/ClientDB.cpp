#include "ClientDB.hpp"

ClientDB::ClientDB( std::string ip_dB, std::string port_dB, 
                    std::string worker_log, std::string worker_pass, 
                    std::string name_sender, std::shared_ptr<tcp::socket> socket, callback_t callback)
{
    __callback_f = callback;
    __end_point = std::make_shared<tcp::endpoint>(tcp::endpoint(net::ip::address::from_string(ip_dB), stoi(port_dB)));
    __socket = socket;
    __buf_recive = new char[BUF_RECIVE_SIZE + 1];
    fill_n(__buf_recive, BUF_RECIVE_SIZE, 0);
    __buf_json_recive = {};
    __parser.reset();
    __worker_login = worker_log;
    __worker_password = worker_pass;
    __name_sender = name_sender;
    __flag_conditions = false;

}
ClientDB::~ClientDB()
{
    cerr << "~ClientDB()" << endl;
    this->stop();
    delete[] __buf_recive;
}
void ClientDB::start()
{
    __flag_connet = false;
    __flag_disconnect = false;
    __socket->async_connect(*__end_point, boost::bind(&ClientDB::__checkConnect, shared_from_this(), boost::placeholders::_1));
}
void ClientDB::stop()
{
    try
    {
        //__socket->shutdown(tcp::socket::shutdown_send);
        //__socket->shutdown(tcp::socket::shutdown_receive);
        if (__socket->is_open())
        {
            __socket->close();
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "stop operation DB " << e.what() << std::endl;
    }
}
void ClientDB::setQuerys( std::queue<std::string> queue_tables, std::queue<std::vector<std::string>> queue_fields, 
                          std::queue<std::string> queue_conditions)
{
    __queue_tables = queue_tables;
    __queue_fields = queue_fields;

    if(!queue_conditions.empty())
    {
        __queue_conditions = queue_conditions;
        //cerr << "cond " << __queue_conditions.size() << " tables" << __queue_tables.size() << endl;
        if (__queue_conditions.size() != __queue_tables.size())
        {
            std::cerr << "setQuerys" << std::endl;
            std::cerr << "ERROR, Not full conditions WHERE( if WHERE is not needed, put " ")";
            this->stop();
        }
        else
        {
            __flag_conditions = true;
        }
    }
}
void ClientDB::setCallback(callback_t callback)
{
    __callback_f = callback;
}
std::map<std::string, std::map<std::string, std::vector<std::string>>> ClientDB::getRespData()
{
    return __resp_data;
}
void ClientDB::__emptyCallback(std::map<std::string, std::map<std::string, std::vector<std::string>>> data)
{
    std::cerr << "ClientDb no Data to Worker" << std::endl;
}
void ClientDB::__checkConnect(const boost::system::error_code& error_code)
{
    if (error_code)
    {
        //cerr << error_code	.what() << endl;
        #ifndef UNIX
            sleep(2);
        #else
            Sleep(2000);
        #endif
        std::cerr << "__checkConnect" << std::endl;
        this->stop();
        this->start();
        return;
    }
    if (__flag_connet == false)
    {
        if (__flag_disconnect == true)
        {
            cerr << "no connect" << endl;
            cerr << "__checkConnect" << endl;
            this->stop();
        }
        else
        {
            __buf_queue_string = boost::json::serialize(json_formatter::database::request::connect(__name_sender, __worker_login, __worker_password));
            __socket->async_send(boost::asio::buffer(__buf_queue_string, __buf_queue_string.size()), boost::bind(&ClientDB::__sendCommand, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
        }
    }
    else
    {
        __queryGenerator();
    }
}
void ClientDB::__queryGenerator()
{
    if (__flag_disconnect == false)
    {
        if (!__queue_tables.empty())
        {
            __table_name_send = __queue_tables.front();
            __fields_name_send = __queue_fields.front();
            __queue_tables.pop();
            __queue_fields.pop();
            std::string query = "SELECT ";
            for (size_t i = 0; i < __fields_name_send.size() - 1; i++)
            {
                query += __fields_name_send[i] + ", ";
            }
            query += __fields_name_send[__fields_name_send.size() - 1];
            query += " FROM " + __table_name_send;
            if (__flag_conditions)
            {
                query += " ";
                std::string conditions = __queue_conditions.front();
                __queue_conditions.pop();
                query += conditions;
                if (__queue_conditions.empty())
                {
                    __flag_conditions = false;
                }
            }
            //cerr << query << endl;
            __buf_queue_string = boost::json::serialize(json_formatter::database::request::query(__name_sender, json_formatter::database::QUERY_METHOD::SELECT, __fields_name_send, query));
            __socket->async_send(boost::asio::buffer(__buf_queue_string, __buf_queue_string.size()), boost::bind(&ClientDB::__sendCommand, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
        }
        else
        {
            __buf_queue_string = boost::json::serialize(json_formatter::database::request::disconnect(__name_sender));
            __socket->async_send(boost::asio::buffer(__buf_queue_string, __buf_queue_string.size()), boost::bind(&ClientDB::__sendCommand, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
        }
    }
    else
    {
        std::cerr << "__queryGenerator()" << std::endl;
        __callback_f(__resp_data);
        this->stop();
    }
}
void ClientDB::__sendCommand(const boost::system::error_code& error_code, size_t bytes_send)
{
    if (error_code)
    {
        std::cerr << "sendConnect" << error_code.what() << std::endl;
        this->stop();
        this->start();
        return;
    }
    static size_t temp_bytes_send = 0;
    /*if (__sendCheck(bytesSend, tempBytesSend, boost::bind(&Client::__sendConnect, shared_from_this(), boost::lambda2::_1, boost::lambda2::_2)) == __CHECK_STATUS::FAIL) {
        return;
    }*/
    temp_bytes_send += bytes_send;
    if (__buf_queue_string.size() != temp_bytes_send) {
        __socket->async_send(boost::asio::buffer(__buf_queue_string.c_str() + temp_bytes_send, (__buf_queue_string.size() - temp_bytes_send)),
            boost::bind(&ClientDB::__sendCommand, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
        return;
    }
    temp_bytes_send = 0;
    __buf_queue_string.clear();
    __socket->async_receive(net::buffer(__buf_recive, BUF_RECIVE_SIZE), boost::bind(&ClientDB::__reciveCommand, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
}

void CleintDB::__reciveCommand(const boost::system::error_code& error_code, size_t bytes_recive)
{
    if (error_code)
    {
        std::cerr << "reciveCommand " << error_code.what() << std::endl;
        this->stop();
        this->start();
        return;
    }
    /*if (__reciveCheck(bytesRecive, boost::bind(&Client::__reciveCommand, shared_from_this(), boost::lambda2::_1, boost::lambda2::_2)) == __CHECK_STATUS::FAIL)
    {
        return;
    }*/
    try {
        __parser.write(__buf_recive, bytes_recive);
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    std::fill_n(__buf_recive, BUF_RECIVE_SIZE, 0);

    if (!__parser.done()) {
        std::cerr << "connectAnalize json not full" << std::endl;
        __socket->async_receive(boost::asio::buffer(__buf_recive, BUF_RECIVE_SIZE), boost::bind(&ClientDB::__reciveCommand, shared_from_this(),
            boost::placeholders::_1, boost::placeholders::_2));
        return;
    }
    try {
        __parser.finish();
        __buf_json_recive = __parser.release();
        __parser.reset();
    }
    catch (std::exception& e) {
        __parser.reset();
        __buf_json_recive = {};
        std::cerr << "_reciveCheck " << e.what() << std::endl;
        return;
    }
    __commandAnalize(error_code);
}
void Cliet_DB::__commandAnalize(const boost::system::error_code& error_code)
{
    boost::json::value target = __buf_json_recive.at("target");
    //cerr << __buf_json_recive << endl;
    if (target == "connect")
    {
        boost::json::value status = __buf_json_recive.at("response").at("status");
        //cout << "status " << status << endl;
        {
            if (status != "success")
            {
                __flag_disconnect = true;
                __checkConnect(error_code);
            }
            else
            {
                __flag_connet = true;
                __flag_disconnect = false;
                __checkConnect(error_code);

            }
        }
    }
    else if (target == "disconnect")
    {
        boost::json::value status = __buf_json_recive.at("response").at("status");
        if (status == "success")
        {
            __flag_disconnect = true;
            __checkConnect(error_code);
        }
        else
        {
            __flag_disconnect = false;
            __checkConnect(error_code);
        }
    }
    else if (target == "db_query")
    {
        std::map<std::string, std::vector<std::string>> bufRespMap;
        std::vector<std::string> valueMap;
        bool whronge = false;
        try
        {
            whronge = false;
            for (size_t i = 0; i < __fields_name_send.size(); i++)
            {
                valueMap.clear();
                boost::json::array valueJson = __buf_json_recive.at("response").at(__fields_name_send[i]).as_array();
                for (size_t j = 0; j < valueJson.size(); j++)
                {
                    valueMap.push_back(boost::json::serialize(valueJson[j]));
                }
                bufRespMap[__fields_name_send[i]] = valueMap;
            }
            __resp_data[__table_name_send] = bufRespMap;
        }
        catch (std::exception& e)
        {
            whronge = true;
            //cout << "No " << __table_name_send << endl;
        }
        __checkConnect(error_code);
    }
}