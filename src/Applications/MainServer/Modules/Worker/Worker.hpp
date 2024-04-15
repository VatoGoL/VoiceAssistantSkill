#pragma once
#include <iostream>
#include <csignal>
#include <boost/bind.hpp>
#include <boost/lambda2.hpp>
#include <boost/asio.hpp>
#include <boost/json.hpp>
#include <queue>
#include "../Smart_Lift/Smart_lift/GlobalModules/JSONFormatter/JSONFormatter.hpp"
//using namespace std;
typedef std::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr;

namespace worker_server {
    const short NO_ID = -1;
    enum WORKER_T {
        WORKER_MARUSIA_T = 1
    };
    
    class ISession {
    protected:
        enum _CHECK_STATUS {
            SUCCESS = 1,
            FAIL
        };
        typedef std::function<void(boost::system::error_code, std::size_t)> _handler_t;

        static const int _BUF_RECIVE_SIZE = 2048;
        const int _PING_TIME = 10;
        std::string _id;
        std::string _sender;
        std::string _buf_send;
        char *_buf_recive;
        boost::json::stream_parser _json_parser;
        std::queue<boost::json::value> _buf_json_recive;
        bool _next_recive = false;
        bool _ping_success = false;
        bool _is_live = false;
        
        virtual void _autorization() = 0;
        virtual _CHECK_STATUS _reciveCheck(const size_t& count_recive_byte, _handler_t&& handler) = 0;
        virtual _CHECK_STATUS _sendCheck(const size_t& count_send_byte, size_t& temp_send_byte, _handler_t&& handler) = 0;
        virtual void _sendCommand(const boost::system::error_code& error, std::size_t count_send_byte) = 0;
        virtual void _reciveCommand(const boost::system::error_code& error, std::size_t count_recive_byte) = 0;
        virtual void _ping(const boost::system::error_code& error) = 0;
        virtual void _analizePing() = 0;
        virtual void _deadPing(const boost::system::error_code& error) = 0;
    public:
        ISession();
        virtual ~ISession();
        
        virtual void start() = 0;
        virtual void stop() = 0;
        virtual bool isLive() = 0;
        virtual std::string getId() = 0;
    };

    /*-----------------------------------------------------------------------------------*/
    class Session : public std::enable_shared_from_this<Session>, protected ISession {
    public:
        enum COMMAND_CODE_MQTT {
            MOVE_LIFT = 1
        };
        enum COMMAND_CODE_MARUSIA {
            MARUSIA_STATION_REQUEST = 1
        };
    private:
        void __emptyCallback(boost::json::value data);
    protected:
        typedef std::function<void(boost::json::value)> _callback_t;

        boost::asio::ip::tcp::socket _socket;
        boost::asio::deadline_timer _ping_timer;
        boost::asio::deadline_timer _dead_ping_timer;
        std::shared_ptr<std::shared_ptr<std::map<std::string, std::vector<std::string>>>> _sp_db_worker_ids;
        _callback_t _callback;

        virtual void _autorization() override;
        virtual _CHECK_STATUS _reciveCheck(const size_t& count_recive_byte, _handler_t&& handler) override;
        virtual _CHECK_STATUS _sendCheck(const size_t& count_send_byte, size_t& temp_send_byte, _handler_t&& handler) override;
        virtual void _sendCommand(const boost::system::error_code& error, std::size_t count_send_byte) override;
        virtual void _reciveCommand(const boost::system::error_code& error, std::size_t count_recive_byte) override;
        virtual void _commandAnalize();
        virtual void _ping(const boost::system::error_code& error) override;
        virtual void _analizePing() override;
        virtual void _deadPing(const boost::system::error_code& error) override;
    public:
        Session(std::string sender, boost::asio::ip::tcp::socket& socket, std::shared_ptr<std::shared_ptr<std::map<std::string, std::vector<std::string>>>> sp_db_worker_ids, boost::asio::deadline_timer ping_timer, boost::asio::deadline_timer dead_ping_timer);
        virtual ~Session();

        virtual void start() override;
        virtual void stop() override;
        virtual bool isLive() override;
        virtual std::string getId() override;

        virtual void startCommand(COMMAND_CODE_MQTT command_code, void* command_parametr, _callback_t callback);
        virtual void startCommand(COMMAND_CODE_MARUSIA command_code, void* command_parametr, _callback_t callback);
    };
    /*-----------------------------------------------------------------------------------*/
    

    /*-----------------------------------------------------------------------------------*/
    
    class SessionMarusia: public Session {
    private:
        void __staticMessage();
        void __moveLift();
    protected:
        virtual void _commandAnalize() override;
    public:
        struct marussia_station_request_t {
            std::string station_id = "";
            boost::json::value body = {};
        };
        SessionMarusia(std::string sender, boost::asio::ip::tcp::socket& socket, std::shared_ptr<std::shared_ptr<std::map<std::string, std::vector<std::string>>>> sp_db_worker_marusia,boost::asio::deadline_timer ping_timer, boost::asio::deadline_timer dead_ping_timer);
        virtual ~SessionMarusia();
        void startCommand(COMMAND_CODE_MARUSIA command_code, void* command_parametr, _callback_t callback);
    };

    /*-----------------------------------------------------------------------------------*/
    
    class Server: public std::enable_shared_from_this<Server> {
    private:
        const int __TIME_KILL = 30;
        WORKER_T __worker_type;
        std::string __sender;
        std::shared_ptr <boost::asio::ip::tcp::acceptor> __acceptor;
        std::shared_ptr <boost::asio::io_context> __context;
        std::shared_ptr<std::vector<std::shared_ptr<Session>>> __sessions;
        std::shared_ptr<boost::asio::deadline_timer> __kill_timer;
        std::shared_ptr<std::shared_ptr<std::map<std::string, std::vector<std::string>>>> __sp_db_worker_ids;

        void __accept();
        void __killSession(const boost::system::error_code& error);
    public:
        Server(std::shared_ptr < boost::asio::io_context> io_context, unsigned short port, WORKER_T worker_type, std::string sender = "Main_server");
        ~Server();
        void start(std::shared_ptr<std::shared_ptr<std::map<std::string, std::vector<std::string>>>> sp_db_worker_ids);
        void stop();
        std::shared_ptr<std::vector<std::shared_ptr<Session>>> getSessions();
    };
}