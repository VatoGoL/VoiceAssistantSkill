#include "Worker.hpp"

using namespace worker_server;

//-------------------------------------------------------------//

Server::Server(std::weak_ptr < boost::asio::io_context> io_context, unsigned short port, WORKER_T worker_type,std::string sender){
    __worker_type = worker_type;
    __sender = sender;
    __context = io_context;
	__acceptor = std::make_shared<boost::asio::ip::tcp::acceptor>(*__context.lock(), boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port));
    __kill_timer = std::make_shared<boost::asio::deadline_timer>(*__context.lock());
    __sessions = std::make_shared<std::vector<std::shared_ptr<Session>>>();
}

Server::~Server() {
	this->stop();
}

void Server::start() 
{
	__accept();
    __kill_timer->expires_from_now(boost::posix_time::seconds(__TIME_KILL));
    __kill_timer->async_wait(boost::bind(&Server::__killSession, shared_from_this(), _1));
}

void Server::stop() {
    for (size_t i = 0, length = __sessions->size(); i < length; i++) {
        __sessions->back()->stop();
    }
    __sessions->clear();
}

void Server::__accept() {
    __acceptor->async_accept([this](boost::system::error_code error, boost::asio::ip::tcp::socket socket)
        {
            if (error) {
                std::cerr << error.message() << std::endl;
                __accept();
            }
            switch (__worker_type) {
            case WORKER_MARUSIA_T:
                if(!__context.expired()){
                    __sessions->push_back(std::make_shared<SessionMarusia>(__sender, socket, boost::asio::deadline_timer(*__context.lock()), boost::asio::deadline_timer(*__context.lock())));
                }
                
                break;
            }
            
            __sessions->back()->start();
            __accept();
        }
    );
}
void Server::__killSession(const boost::system::error_code& error) {
    bool kill = false;
    for (auto i = __sessions->begin(); i != __sessions->end(); i++) {
        if (!(*i)->isLive()) {
            (*i)->stop();
            __sessions->erase(i);
            kill = true;
            break;
        }
    }
    if (kill) {
        __kill_timer->expires_from_now(boost::posix_time::seconds(1));
        __kill_timer->async_wait(boost::bind(&Server::__killSession, shared_from_this(), _1));
    }
    else{
        __kill_timer->expires_from_now(boost::posix_time::seconds(__TIME_KILL));
        __kill_timer->async_wait(boost::bind(&Server::__killSession, shared_from_this(), _1));
    }
}
std::shared_ptr<std::vector<std::shared_ptr<Session>>> Server::getSessions() {
    return __sessions;
}
//-------------------------------------------------------------//

ISession::ISession(){
    _id = std::to_string(NO_ID);
    _buf_send = "";
    _buf_recive = new char[_BUF_RECIVE_SIZE+1];
    std::fill_n(_buf_recive,_BUF_RECIVE_SIZE, 0);
    _sender = "";
    _json_parser.reset();
}
ISession::~ISession() 
{   
    Logger::writeLog("ISession","Destructor",Logger::log_message_t::EVENT,"Session delete");
    delete[] _buf_recive; 
}
const int ISession::_BUF_RECIVE_SIZE = 2048;
const int ISession::_PING_TIME = 10;
//-------------------------------------------------------------//
long Session::MAX_CPU_STAT = 80; 
long Session::MAX_MEM_STAT = 80;
Session::Session(std::string sender, boost::asio::ip::tcp::socket& socket,
    boost::asio::deadline_timer ping_timer, boost::asio::deadline_timer dead_ping_timer) :
    _socket(std::move(socket)),
    _ping_timer(std::move(ping_timer)), _dead_ping_timer(std::move(dead_ping_timer))
{
    _callback = boost::bind(&Session::__emptyCallback, this, _1);
    _sender = sender;
    _cpu_stat = 0;
    _mem_stat = 0;
}
Session::~Session() {
    this->stop();
}
void Session::start() {
    _socket.async_receive(boost::asio::buffer(_buf_recive, _BUF_RECIVE_SIZE), boost::bind(&Session::_reciveCommand, shared_from_this(), _1, _2));
}
void Session::stop() {
    _is_live = false;
    _ping_timer.cancel();
    _dead_ping_timer.cancel();
    try{
        if (_socket.is_open()) {
            _socket.close();
        }
    }
    catch(std::exception &e){
        Logger::writeLog("Session","stop",Logger::log_message_t::ERROR,std::string("stop operation Worker server: ") + e.what());
    }
    
}
long Session::getCPUStat()
{
    return _cpu_stat;
}   
long Session::getMemStat()
{
    return _mem_stat;
}
const boost::json::array& Session::getActiveClientsIp()
{
    return _active_clients_ip;
}
void Session::_autorization() {
    boost::json::value analize_value = _buf_json_recive.front();
    _buf_json_recive.pop();
    try {
        //� ������ ������
        _is_live = true;
        _buf_send = serialize(json_formatter::worker::response::connect(_sender));

        _ping_timer.expires_from_now(boost::posix_time::seconds(ISession::_PING_TIME));
        _ping_timer.async_wait(boost::bind(&Session::_ping, shared_from_this(), _1));
    }
    catch (std::exception& e) {
        Logger::writeLog("Session","__autorization",Logger::log_message_t::ERROR, e.what());
        _buf_send = serialize(json_formatter::worker::response::connect(_sender, json_formatter::ERROR_CODE::CONNECT, "Field Id not found"));
    }
    _socket.async_send(boost::asio::buffer(_buf_send, _buf_send.size()),
        boost::bind(&Session::_sendCommand, shared_from_this(), _1, _2));
}
Session::_CHECK_STATUS Session::_reciveCheck(const size_t& count_recive_byte, _handler_t&& handler)
{
    size_t count_byte, count_byte_write = 0;
    for (; count_byte_write < count_recive_byte;) {
        try {
            count_byte = _json_parser.write_some(_buf_recive + count_byte_write);
        }
        catch (std::exception& e) {
            std::cerr << e.what() << std::endl;
        }

        if (!_json_parser.done()) {
            std::fill_n(_buf_recive, _BUF_RECIVE_SIZE, 0);
            Logger::writeLog("Session","_reciveCheck",Logger::log_message_t::ERROR, "_reciveCheck json not full");
            _socket.async_receive(boost::asio::buffer(_buf_recive, _BUF_RECIVE_SIZE), handler);
            return _CHECK_STATUS::FAIL;
        }
        try {
            _buf_json_recive.push(_json_parser.release());
            _json_parser.reset();
            count_byte_write += count_byte;
        }
        catch (std::exception& e) {
            _json_parser.reset();
            _buf_json_recive = {};
            Logger::writeLog("Session","_reciveCheck",Logger::log_message_t::ERROR, e.what());
            return _CHECK_STATUS::FAIL;
        }
    }
    std::fill_n(_buf_recive, _BUF_RECIVE_SIZE, 0);
    return _CHECK_STATUS::SUCCESS;
    
}
Session::_CHECK_STATUS Session::_sendCheck(const size_t& count_send_byte, size_t& temp_send_byte, _handler_t&& handler)
{
    temp_send_byte += count_send_byte;
    if (_buf_send.size() != temp_send_byte) {
        _socket.async_send(boost::asio::buffer(_buf_send.c_str() + temp_send_byte, (_buf_send.size() - temp_send_byte)), handler);
        return _CHECK_STATUS::FAIL;
    }
    return _CHECK_STATUS::SUCCESS;
}
void Session::_sendCommand(const boost::system::error_code& error, std::size_t count_send_byte)
{
    if (error) {
        Logger::writeLog("Session","_sendCommand",Logger::log_message_t::ERROR, error.what());
        this->stop();
        return;
    }

    if (!_is_live) {
        this->stop();
        return;
    }

    static size_t temp_send_byte = 0;
    if (_sendCheck(count_send_byte, temp_send_byte, boost::bind(&Session::_sendCommand, shared_from_this(), _1, _2)) == _CHECK_STATUS::FAIL) {
        return;
    }
    temp_send_byte = 0;
    _buf_send = "";
    
    if (_next_recive) {
        _next_recive = false;
        _socket.async_receive(boost::asio::buffer(_buf_recive, _BUF_RECIVE_SIZE),
            boost::bind(&Session::_reciveCommand, shared_from_this(), _1, _2));
    }
}
void Session::_reciveCommand(const boost::system::error_code& error, std::size_t count_recive_byte)
{
    if (error) {
        Logger::writeLog("Session","_reciveCommand",Logger::log_message_t::ERROR, error.what());
        this->stop();
        return;
    }

    if (_reciveCheck(count_recive_byte, boost::bind(&Session::_reciveCommand, shared_from_this(), _1, _2)) == _CHECK_STATUS::FAIL)
    {
        return;
    }

    _commandAnalize();
    _buf_json_recive = {};
}
void Session::_ping(const boost::system::error_code& error)
{
    if (error) {
        Logger::writeLog("Session","_ping",Logger::log_message_t::ERROR, error.what());
        this->stop();
        return;
    }
    if(_is_live){
        _buf_send = serialize(json_formatter::worker::request::ping(_sender));
        _next_recive = true;
        _ping_success = false;
        _dead_ping_timer.cancel();
        _dead_ping_timer.expires_from_now(boost::posix_time::seconds(_PING_TIME*2));
        _dead_ping_timer.async_wait(boost::bind(&Session::_deadPing, shared_from_this(), _1));

        _socket.async_send(boost::asio::buffer(_buf_send, _buf_send.size()),
            boost::bind(&Session::_sendCommand, shared_from_this(), _1, _2));
    }
}
void Session::_analizePing()
{
    boost::json::value analise_value = _buf_json_recive.front();
    _buf_json_recive.pop();
    try {
        if (analise_value.at("response").at("status") == "success") {
            _cpu_stat = analise_value.at("response").at("cpu").as_int64();
            _mem_stat = analise_value.at("response").at("mem").as_int64();
            _active_clients_ip = analise_value.at("response").at("clients_ip").get_array(); 
            _ping_success = true;
            _dead_ping_timer.cancel();
            _ping_timer.expires_from_now(boost::posix_time::seconds(_PING_TIME));
            _ping_timer.async_wait(boost::bind(&Session::_ping, shared_from_this(), _1));
        }
        else {
            _is_live = false;
            //cerr << "_analizePing Error response status not equal success, status = " << analise_value.at("response").at("status") << endl;
            this->stop();
            return;
        }
    }
    catch (std::exception& e) {
        Logger::writeLog("Session","_analizePing",Logger::log_message_t::ERROR, std::string("Session stop: ") + e.what());
        this->stop();
        return;
    }
}
void Session::_deadPing(const boost::system::error_code& error) {
    if (!_ping_success) {
        //_dead_ping_timer.cancel();
        _is_live = false;

        if (error) {
            std::cerr << error << std::endl;
        }

        this->stop();
    }
}
bool Session::isLive() {
    return _is_live;
}
void Session::__emptyCallback(boost::json::value data)
{
    std::cerr << "required __emptyCallback" << std::endl;
}
void Session::_commandAnalize() {}
void Session::startCommand(COMMAND_CODE_MARUSIA command_code, void* command_parametr, _callback_t callback) {}
std::string Session::getId(){
    return _id;
}

//-------------------------------------------------------------//

//-------------------------------------------------------------//

SessionMarusia::SessionMarusia(std::string sender, boost::asio::ip::tcp::socket& socket,
    boost::asio::deadline_timer ping_timer, boost::asio::deadline_timer dead_ping_timer) :
    Session(sender, socket, std::move(ping_timer), std::move(dead_ping_timer))
{}
SessionMarusia::~SessionMarusia() {
    this->stop();
}

void SessionMarusia::_commandAnalize() 
{
    try {
        boost::json::value target = _buf_json_recive.front().at("target");
        if (target == "ping") {
            _analizePing();
        }
        else if (target == "static_message") {
            __staticMessage();
        }
        else if (target == "connect") {
            _autorization();
        }
        
    }
    catch (std::exception& e) {
        Logger::writeLog("Session","_commandAnalize",Logger::log_message_t::ERROR, e.what());
    }
}
void SessionMarusia::startCommand(COMMAND_CODE_MARUSIA command_code, void* command_parametr, _callback_t callback)
{
    if (command_code == MARUSIA_STATION_REQUEST && _is_live) {
        _callback = callback;
        marussia_station_request_t* parametr = (marussia_station_request_t*)command_parametr;
        _buf_send = serialize(json_formatter::worker::request::marussia_request(_sender, parametr->station_ip, parametr->body));
        _next_recive = true;
        _socket.async_send(boost::asio::buffer(_buf_send, _buf_send.size()), 
                            boost::bind(&SessionMarusia::_sendCommand, shared_from_this(), _1, _2));
    }
}
void SessionMarusia::__staticMessage() 
{
    _callback(_buf_json_recive.front());
    _buf_json_recive.pop();
}
void SessionMarusia::__moveLift() 
{
    _callback(_buf_json_recive.front());
    _buf_json_recive.pop();
}