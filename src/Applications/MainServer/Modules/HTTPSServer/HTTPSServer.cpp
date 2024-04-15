#include "HTTPSServer.hpp"

using namespace https_server;

//------------------------------------------------------------------------------
void https_server::fail(beast::error_code ec, char const* what) {
    if (ec == net::ssl::error::stream_truncated) { return; }

    std::cerr << what << ": " << ec.message() << "\n";
}
//------------------------------------------------------------------------------

Session::Session(tcp::socket&& socket,
    ssl::context& ssl_ctx,
    std::string path_to_ssl_certificate,
    std::string path_to_ssl_key,
    std::shared_ptr<std::vector<std::shared_ptr<worker_server::Session>>> sessions_mqtt,
    std::shared_ptr<std::vector<std::shared_ptr<worker_server::Session>>> sessions_marussia,
    std::shared_ptr< shared_ptr<map<string, vector<string>>>> sp_db_marusia_station,
    std::shared_ptr< shared_ptr<map<string, vector<string>>>> sp_db_lift_blocks): __stream(std::move(socket), ssl_ctx), __ssl_ctx(ssl_ctx)
{
    __sp_db_marusia_station = sp_db_marusia_station;
    __sp_db_lift_blocks = sp_db_lift_blocks;
    __sessions_marusia = sessions_marussia;
    __sessions_mqtt = sessions_mqtt;
    __path_to_ssl_certificate = path_to_ssl_certificate;
    __path_to_ssl_key = path_to_ssl_key;
    __is_live = false;
    __reconnect_count = 0;
}
Session::~Session(){
    
    cout << "KILL HTTPS SESSION" << endl;
}

void Session::run() {

    cout << "METHOD RUN" << endl;
    net::dispatch( __stream.get_executor(), 
                    beast::bind_front_handler( &Session::__onRun, shared_from_this() ));
}
void Session::__onRun() {
    cout << "METHOD OnRUN" << endl;
    // Set the timeout.
    beast::get_lowest_layer(__stream).expires_after(std::chrono::seconds(30));
    // Perform the SSL handshake
    __stream.async_handshake( ssl::stream_base::server, 
                              beast::bind_front_handler( &Session::__onHandshake, shared_from_this() ));
}
void Session::__onHandshake(beast::error_code ec) {
    /*тут нужно добавить перечтение ssl сертификата*/
    cout << "METHOD Handshake" << endl;
    if (ec) { 

        if(load_server_certificate(__ssl_ctx,__path_to_ssl_certificate, __path_to_ssl_key) == -1){
            return fail(ec, "Load Sertificate error"); 
        }
        if(__reconnect_count < __RECONNECT_MAX){
            __reconnect_count++;
            this->run();
        }
        else{
            __is_live = false;
        }
        
        return;
    }
    __is_live = true;
    __doRead();
}
void Session::__doRead() {
    cout << "METHOD doRead" << endl;
    __req = {};

    // Set the timeout.
    beast::get_lowest_layer(__stream).expires_after(std::chrono::seconds(30));

    // Read a request
    http::async_read(__stream, __buffer, __req,
                    beast::bind_front_handler( &Session::__onRead, shared_from_this() ));
}
void Session::__onRead(beast::error_code ec, std::size_t bytes_transferred) 
{
    cout << "METHOD onRead" << endl;
    boost::ignore_unused(bytes_transferred);

    // This means they closed the connection
    if (ec == http::error::end_of_stream) { return __doClose(); }

    if (ec) 
    { //return fail(ec, "read");
        __is_live = false;
        return;
    }

    // Send the response
    
    __analizeRequest();
}
void Session::__sendResponse(http::message_generator&& msg) {
    bool keep_alive = msg.keep_alive();

    // Write the response
    beast::async_write( __stream, std::move(msg),
                        beast::bind_front_handler( &Session::__onWrite, shared_from_this(), keep_alive));
}
void Session::__onWrite(bool keep_alive, beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    if (ec) { 
        __is_live = false;
        return fail(ec, "write");
    }
        

    if (!keep_alive)
    {
        // This means we should close the connection, usually because
        // the response indicated the "Connection: close" semantic.
        return __doClose();
    }

    //изменения
    __is_live = false;
    // Read another request
    //__doRead();
}
void Session::__doClose() {
    __is_live = false;
    // Set the timeout.
    beast::get_lowest_layer(__stream).expires_after(std::chrono::seconds(30));

    // Perform the SSL shutdown
    __stream.async_shutdown(
        beast::bind_front_handler(
            &Session::__onShutdown,
            shared_from_this()));
}
void Session::__onShutdown(beast::error_code ec) {
    __is_live = false;
    if (ec) { return fail(ec, "shutdown"); }
    // At this point the connection is closed gracefully
}
bool Session::isLive() {
    return __is_live;
}

void Session::__analizeRequest()
{
    
    if (__req.method() != http::verb::post && __req.method() != http::verb::options) {

        std::cout << std::endl << __req.method() << std::endl<< __req << std::endl << __req.body() << std::endl;
        __req = {};
        __sendResponse(__badRequest("Method not equal OPTIONS OR POST\n"));
        return;
    }

    if (__req.method() == http::verb::options) {
        beast::string_view response_data = "POST, OPTIONS";
        http::response<http::string_body> res{ std::piecewise_construct,
            std::make_tuple(""),
            std::make_tuple(http::status::ok, __req.version()) };
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::access_control_allow_origin, "*");
        res.set(http::field::access_control_allow_methods, response_data.data());
        res.set(http::field::access_control_allow_headers, "Content-Type, Accept");
        //res.set(http::field::content_length, "0");
        res.prepare_payload();
        res.keep_alive(true);
        __req = {};
        __sendResponse((http::message<false, http::string_body, http::fields>)res);
        return;
    }

    //POST
    beast::error_code err_code;

    try {
        __parser.reset();
        __parser.write(__req.body(), err_code);
        if (err_code) {
            __req = {};
            __sendResponse(__badRequest("parse JSON error"));
            return;
        }
        if (!__parser.done()) {
            __req = {};
            __sendResponse(__badRequest("JSON not full"));
            return;
        }
    }
    catch (std::bad_alloc const& e) {
        __req = {};
        __sendResponse(__badRequest(std::string("Bad alloc JSON: ") + e.what()));
        return;
    }
    __body_request = __parser.release();

    if (__sessions_marusia->size() == 0) {
        cerr << "__session_marussia size = 0" << endl;
        __callbackWorkerMarussia({});
        return;
    }
    try {
        bool search_successful = false;
        
        string application_id = "\"" + string(__body_request.at("session").at("application").at("application_id").as_string().c_str()) + "\"";
        if ((*__sp_db_marusia_station)->at("ApplicationId")[__pos_ms_field] != application_id) {
            __pos_ms_field = 0;
            for (auto i = (*__sp_db_marusia_station)->at("ApplicationId").begin(), end = (*__sp_db_marusia_station)->at("ApplicationId").end(); i != end; i++) {
                if (application_id == (*i)) {
                    search_successful = true;
                    break;
                }
                __pos_ms_field++;
            }
        }
        else { search_successful = true; }
        
        if (!search_successful) {
            throw invalid_argument(string("application id not found: " + application_id).c_str());
        }
        /*-----------------------------------------*/
        search_successful = false;

        string worker_id = (*__sp_db_marusia_station)->at("WorkerId").at(__pos_ms_field);
        string second_worker_id = (*__sp_db_marusia_station)->at("SecondWorkerId").at(__pos_ms_field);
        
        string temp_var;
        try {
            temp_var = __sessions_marusia->at(__pos_worker_marusia)->getId();
            if (temp_var == worker_id || temp_var == second_worker_id) {
                search_successful = true;
            }
            else {
                throw invalid_argument("");
            }
        }
        catch (exception& e) {
            temp_var = "";
            __pos_worker_marusia = 0;
            for (auto i = __sessions_marusia->begin(), end = __sessions_marusia->end(); i != end; i++) {
                temp_var = (*i)->getId();
                if (temp_var == worker_id || temp_var == second_worker_id) {
                    search_successful = true;
                    break;
                }
                __pos_worker_marusia++;
            }
        }
        
        if (!search_successful) {
            __callbackWorkerMarussia({});
            return;
        }
        
        /*-----------------------------------------*/
        /*-----------------------------------------*/

        search_successful = false;
        string lb_id = (*__sp_db_marusia_station)->at("LiftBlockId")[__pos_ms_field];

        if ((*__sp_db_lift_blocks)->at("Id")[__pos_lb_field] != lb_id) {
            __pos_lb_field = 0;
            for (auto i = (*__sp_db_lift_blocks)->at("Id").begin(), end = (*__sp_db_lift_blocks)->at("Id").end(); i != end; i++) {
                if (lb_id == (*i)) {
                    search_successful = true;
                    break;
                }
                __pos_lb_field++;
            }
        }

        search_successful = false;
        worker_id = (*__sp_db_lift_blocks)->at("WorkerLuId").at(__pos_lb_field);
        second_worker_id = (*__sp_db_lift_blocks)->at("SecondWorkerLuId").at(__pos_lb_field);
        try {
            temp_var = __sessions_mqtt->at(__pos_worker_lu)->getId();

            if (temp_var == worker_id || temp_var == second_worker_id) {
                search_successful = true;
            }
            else {
                throw invalid_argument("");
                //throw exception("");
            }
        }
        catch (exception& e) {
            temp_var = "";
            __pos_worker_lu = 0;
            for (auto i = __sessions_mqtt->begin(), end = __sessions_mqtt->end(); i != end; i++) {
                temp_var = (*i)->getId();
                if (temp_var == worker_id || temp_var == second_worker_id) {
                    search_successful = true;
                    break;
                }
                __pos_worker_lu++;
            }
        }

        if (!search_successful) {
            __callbackWorkerMarussia({});
            return;
        }

        /*-----------------------------------------*/
        __request_marusia = {};
        __request_marusia.body = __body_request;
        __request_marusia.station_id = __body_request.at("session").at("application").at("application_id").as_string();
        __sessions_marusia->at(__pos_worker_marusia)->startCommand(worker_server::Session::COMMAND_CODE_MARUSIA::MARUSIA_STATION_REQUEST, (void*)&__request_marusia,
            boost::bind(&Session::__callbackWorkerMarussia, this, _1));
    }
    catch (exception& e) {
        cerr << "__analizeRequest: " << e.what() << endl;
        __callbackWorkerMarussia({{"target", "application_not_found"}});
    }
    
}
http::message_generator Session::__badRequest(beast::string_view why) {
    __is_live = false;
    std::cout << "ERROR " << why << std::endl;
    http::response<http::string_body> res{http::status::bad_request, __req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "application/json;charset=utf-8");
    res.keep_alive(false);
    res.body() = std::string(why);
    res.prepare_payload();
    return res;
}
void Session::__callbackWorkerMarussia(boost::json::value data) {
    boost:json::value target;
    boost::json::object response_data = {};
    __request_mqtt = {};

    try {
        target = data.at("target");
    }
    catch (exception& e) {
        cout << "__callbackWorkerMarussia [target]: " << e.what();
        target = "error";
    };
    /*------------*/
    try {
        if (target == "static_message") {
            response_data = data.at("response").at("response_body").as_object();
        }
        else if (target == "move_lift") {
            if (__sessions_mqtt->size() == 0) {
                throw invalid_argument("session mqtt size = 0");
            }
            __request_mqtt.station_id = data.at("response").at("station_id").as_string();
            __request_mqtt.floor = stoi(data.at("response").at("mqtt_command").at("value").as_string().c_str());
            __request_mqtt.lift_block_id = data.at("response").at("mqtt_command").at("lb_id").as_string();

            __sessions_mqtt->at(__pos_worker_lu)->startCommand(worker_server::Session::COMMAND_CODE_MQTT::MOVE_LIFT, (void*)&(__request_mqtt),
                boost::bind(&Session::__callbackWorkerMQTT, this, _1));
            return;
        }
        else if(target == "application_not_found"){}
        else {
            target = "error";
        }
    }
    catch (exception& e) {
        cerr << "__callbackWorkerMarussia [target analize]: " << e.what() << endl;
        target = "error";
    };
    /*------------*/
    if (target == "error") {
        response_data =
        {
            {"text", "Извините пожалуйста сервис временно недоступен"},
            {"tts", "Извините пожалуйста сервис временно недоступен"},
            {"end_session", false}, //mb ne nado zakrivat
        };
    }
    else if(target == "application_not_found"){
        response_data =
        {
            {"text", "Здравствуйте, я скилл Умный лифт, я обладаю двумя наборами команд. Первая это поднятие лифта на нужный этаж. Для этого просто скажите \"подними на нужный этаж\", \"на нужный этаж\" и т.п. Второе это информирование о местах и событиях которые происходят рядом. Для примера, спросите что есть рядом, где аквапарк, где массажист, есть ли врач и тому подобное. Предупреждаю, если ваше устройство не подключено к базе данных, вы не сможете воспользоваться функционалом скилла во избежание проблем с использованием лифтов."},
            {"tts", "Здравствуйте, я скилл Умный лифт, я обладаю двумя наборами команд. Первая это поднятие лифта на нужный этаж. Для этого просто скажите \"подними на нужный этаж\", \"на нужный этаж\" и т.п. Второе это информирование о местах и событиях которые происходят рядом. Для примера, спросите что есть рядом, где аквапарк, где массажист, есть ли врач и тому подобное. Предупреждаю, если ваше устройство не подключено к базе данных, вы не сможете воспользоваться функционалом скилла во избежание проблем с использованием лифтов."},
            {"end_session", false}, //mb ne nado zakrivat
        };
    }

    __constructResponse(response_data);
}
void Session::__callbackWorkerMQTT(boost::json::value data) {
    boost:json::value target;
    boost::json::object response_data = {};
    u8string text;
    string result_text;

    try {
        target = data.at("target");
    }
    catch (exception& e) {
        cout << "__callbackWorkerMarussia [target]: " << e.what();
        target = "error";
    };

    /*------------*/
    try {
        if (target == "mqtt_message") {
            if (data.at("response").at("status") == "success") {
                result_text = "Могу ли я ещё чем-нибудь помочь?";
            }
            else {
                result_text = "Извините пожалуйста сервис временно недоступен";
            }
            response_data =
            {
                {"text", result_text},
                {"tts", result_text},
                {"end_session", false}, 
            };
        }
        else {
            target = "error";
        }
    }
    catch (exception& e) {
        cerr << "__callbackWorkerMarussia [target analize]: " << e.what() << endl;
        target = "error";
    };
    //string_body a;

    /*------------*/
    if (target == "error") {
        result_text = "Извините пожалуйста сервис временно недоступен";
        response_data =
        {
            {"text", result_text},
            {"tts", result_text},
            {"end_session", true}, //mb ne nado zakrivat
        };
    }

    __constructResponse(response_data);
}
void Session::__constructResponse(boost::json::object response_data) {

    __body_response["response"] = response_data;

    __body_response["session"] =
    {
        {"session_id", __body_request.at("session").at("session_id")},
        {"user_id", __body_request.at("session").at("application").at("application_id")},
        {"message_id", __body_request.at("session").at("message_id").as_int64()}
    };
    __body_response["version"] = "1.0";

    http::response<http::string_body> response{
        std::piecewise_construct,
            std::make_tuple(""),
            std::make_tuple(http::status::ok, __req.version()) };
    response.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    response.set(http::field::content_type, "application/json; charset=utf-8; locale=ru-RU;");
    response.set(http::field::access_control_allow_origin, "*");
    response.keep_alive(__req.keep_alive());
    response.body() = serialize(__body_response);
    response.prepare_payload();
    __req = {};
    __body_response = {};
    __body_request = {};

    __sendResponse((http::message<false, http::string_body, http::fields>)response);
}

//------------------------------------------------------------------------------

Listener::Listener( net::io_context& io_ctx,
                    ssl::context& ssl_ctx,
                    std::string path_to_ssl_certificate,
                    std::string path_to_ssl_key,
                    unsigned short port,
                    std::shared_ptr<std::vector<std::shared_ptr<worker_server::Session>>> sessions_mqtt,
                    std::shared_ptr<std::vector<std::shared_ptr<worker_server::Session>>> sessions_marussia): 
                                                                         __io_ctx(io_ctx),
                                                                         __ssl_ctx(ssl_ctx)
{
    
    __acceptor = make_shared<tcp::acceptor>(__io_ctx, tcp::endpoint(tcp::v4(), port));

    __sessions_mqtt = sessions_mqtt;
    __sessions_marussia = sessions_marussia;
    __timer_kill = std::make_shared<boost::asio::deadline_timer>(io_ctx);
    __sessions = std::make_shared<std::vector<std::shared_ptr<https_server::Session>>>();
    __path_to_ssl_certificate = path_to_ssl_certificate;
    __path_to_ssl_key = path_to_ssl_key;
} 
void Listener::start(std::shared_ptr< shared_ptr<map<string, vector<string>>>> sp_db_marusia_station, std::shared_ptr< shared_ptr<map<string, vector<string>>>> sp_db_lift_blocks) {
    __sp_db_marusia_station = sp_db_marusia_station;
    __sp_db_lift_blocks = sp_db_lift_blocks;
    __acceptor->async_accept(beast::bind_front_handler(&Listener::__onAccept, shared_from_this()));
    __timer_kill->expires_from_now(boost::posix_time::seconds(__TIME_DEAD_SESSION));
    __timer_kill->async_wait(beast::bind_front_handler(&Listener::__killSession,shared_from_this()));
}

void Listener::__killSession(beast::error_code ec) {
    bool kill = false;
    for (auto i = __sessions->begin(); i != __sessions->end(); i++) {
        if (!(*i)->isLive()) {
            __sessions->erase(i);
            kill = true;
            break;
        }
    }
    if (kill) {
        __timer_kill->expires_from_now(boost::posix_time::seconds(1));
        __timer_kill->async_wait(beast::bind_front_handler(&Listener::__killSession, shared_from_this()));
    }
    else {
        __timer_kill->expires_from_now(boost::posix_time::seconds(__TIME_DEAD_SESSION));
        __timer_kill->async_wait(beast::bind_front_handler(&Listener::__killSession, shared_from_this()));
    }
}
void Listener::__onAccept(beast::error_code ec, tcp::socket socket) {
    if (ec)
    {
        fail(ec, "accept");
        return; // To avoid infinite loop
    }
    else
    {
        // Create the session and run it
        __sessions->push_back(std::make_shared<https_server::Session>(
                                std::move(socket),
                                __ssl_ctx,
                                __path_to_ssl_certificate,
                                __path_to_ssl_key,
                                __sessions_mqtt, __sessions_marussia, 
                                __sp_db_marusia_station, __sp_db_lift_blocks));
        __sessions->back()->run();
       /* std::make_shared<https_server::Session>(
                                std::move(socket),
                                __ssl_ctx,
                                __path_to_ssl_certificate,
                                __path_to_ssl_key,
                                __sessions_mqtt, __sessions_marussia, 
                                __sp_db_marusia_station, __sp_db_lift_blocks)->run();*/
    }

    // Accept another connection
    __acceptor->async_accept(net::make_strand(__io_ctx), beast::bind_front_handler(&Listener::__onAccept, shared_from_this()));
}