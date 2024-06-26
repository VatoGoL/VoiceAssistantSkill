#pragma once

#define BOOST_BIND_GLOBAL_PLACEHOLDERS

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>
#include <boost/json.hpp>
#include <exception>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <string>
#include "../Worker/Worker.hpp"
#include "../SSLSertificate/SSLSertificate.hpp"

namespace https_server {

    namespace json = boost::json;
    namespace beast = boost::beast;         // from <boost/beast.hpp>
    namespace http = beast::http;           // from <boost/beast/http.hpp>
    namespace net = boost::asio;            // from <boost/asio.hpp>
    namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
    using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

    //------------------------------------------------------------------------------
    void fail(beast::error_code ec, char const* what);
    //------------------------------------------------------------------------------

    class Session : public std::enable_shared_from_this<Session>
    {
        ssl::context& __ssl_ctx;
        beast::ssl_stream<beast::tcp_stream> __stream;
        std::string __ip_client;
        beast::flat_buffer __buffer;
        http::request<http::string_body> __req;
        json::stream_parser __parser;
        json::value __body_request;
        json::object __body_response;
        bool __is_live = false;

        std::weak_ptr<std::vector<std::shared_ptr<worker_server::Session>>> __sessions_marusia;

        long __pos_ms_field = 0;
        long __pos_lb_field = 0;
        long __pos_worker_marusia = 0;
        long __pos_worker_lu = 0;

        worker_server::SessionMarusia::marussia_station_request_t __request_marusia;

        std::string __path_to_ssl_certificate;
        std::string __path_to_ssl_key;

        static const int __RECONNECT_MAX;
        int __reconnect_count = 0;
    public:
        explicit Session(tcp::socket&& socket,
            ssl::context& ssl_ctx,
            std::string ip_client,
            std::string path_to_ssl_certificate,
            std::string path_to_ssl_key,
            std::weak_ptr<std::vector<std::shared_ptr<worker_server::Session>>> sessions_marussia);
        ~Session();
        void run();
        bool isLive();
    private:
        void __onRun();
        void __onHandshake(beast::error_code ec);
        void __doRead();
        void __onRead(beast::error_code ec,
                      std::size_t bytes_transferred);
        void __sendResponse(http::message_generator&& msg);
        void __onWrite(bool keep_alive,
                       beast::error_code ec,
                       std::size_t bytes_transferred);
        void __doClose();
        void __onShutdown(beast::error_code ec);

        void __analizeRequest();
        http::message_generator __badRequest(beast::string_view why);
        void __callbackWorkerMarussia(boost::json::value data);
        void __constructResponse(const boost::json::object& response_data, const boost::json::object& session);
    };

    //------------------------------------------------------------------------------

    class Listener : public std::enable_shared_from_this<Listener>
    {
        const int __TIME_DEAD_SESSION = 5;
        net::io_context& __io_ctx;
        ssl::context& __ssl_ctx;
        std::shared_ptr<tcp::acceptor> __acceptor;
        std::shared_ptr<std::vector<std::shared_ptr<https_server::Session>>> __sessions;
        std::shared_ptr<boost::asio::deadline_timer> __timer_kill;

        std::weak_ptr<std::vector<std::shared_ptr<worker_server::Session>>> __sessions_marussia;

        std::string __path_to_ssl_certificate;
        std::string __path_to_ssl_key;
    public:
        Listener( net::io_context& io_ctx,
                  ssl::context& ssl_ctx,
                  std::string path_to_ssl_certificate,
                  std::string path_to_ssl_key,
                  unsigned short port,
                  std::weak_ptr<std::vector<std::shared_ptr<worker_server::Session>>> sessions_marussia);
        void start();
    
    private:
        void __onAccept(beast::error_code ec, tcp::socket socket);
        void __killSession(beast::error_code ec);
    };   
}