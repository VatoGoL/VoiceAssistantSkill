#include "Worker.hpp"

void Worker::__сallback(std::map<std::string, std::map<std::string, std::vector<std::string>>> data)
{
    setDbInfo(data);
}
Worker::Worker( std::map<std::string, std::string> conf_info, net::io_context& ioc, std::shared_ptr<Logger> lg):
                __ioc(ioc),
                callback(boost::bind(&Worker::__сallback, this, boost::placeholders::_1))
{
    __socket = std::make_shared<tcp::socket>(__ioc);
    __buf_recive = new char[BUF_RECIVE_SIZE + 1];
    __buf_send = "";
    __buf_json_recive = {};
    __parser.reset();
    __config_info = conf_info;
    __timer = std::make_shared<net::deadline_timer>(__ioc);
    __log = lg;
    __flag_disconnect = false;
}
Worker::~Worker()
{
    this->stop();
    delete[] __buf_recive;
}
void Worker::start()
{
    __log->writeLog(0, __name, "start_server");
    __log->writeTempLog(0, __name, "start_server");
    __timer->expires_from_now(boost::posix_time::hours(24));
    __timer->async_wait(boost::bind(&Worker::__resetTimer, shared_from_this()));
    try
    {
    __ip_ms = __config_info.at("Main_server_ip");
    __port_ms = __config_info.at("Main_server_port");
    __worker_id = __config_info.at("Id");
    __ip_db = __config_info.at("BD_ip");
    __port_db = __config_info.at("BD_port");
    __db_log = __config_info.at("BD_login");
    __db_pas = __config_info.at("BD_password");
    __socket_dB = std::make_shared<tcp::socket>(__ioc);
    __db_client = std::make_shared<ClientDB>(__ip_db, __port_db, __db_log, __db_pas, __name, __socket_dB, callback);
    __end_point = std::make_shared<tcp::endpoint>(tcp::endpoint(net::ip::address::from_string(__ip_ms), stoi(__port_ms)));
    std::cerr << "IP " << net::ip::address::from_string(__ip_ms) << " port " << stoi(__port_ms) << std::endl;


    }
    catch (std::exception& e)
    {
    std::cerr << e.what() << std::endl;
    this->stop();
    return;
    }
    __connectToBd();
    __connectToMS();
}
void Worker::stop()
{
    try
    {
        if (__socket->is_open())
        {
            __socket->close();
        }
        __log->writeLog(0, "WorkerMS", "End_Connect");
        __log->writeTempLog(0, "WorkerMs", "End_Connect");
    }
    catch (std::exception& e)
    {
        std::cerr << "worker Marussia " << e.what() << std::endl;
    }
}
void Worker::setDbInfo(std::map <std::string, std::map<std::string, std::vector<std::string>>> data)
{
    __db_info = data;
}
void Worker::__connectToMS()
{
    __socket->async_connect(*__end_point, boost::bind(&Worker::__sendConnect, this, boost::placeholders::_1));
}
void Worker::__sendConnect(const boost::system::error_code& eC)
{
    if (eC)
    {
        __log->writeLog(3, __name, "Error_failed_to_connect");
        __log->writeTempLog(3, __name, "Error_failed_to_connect");
        cerr << "connect" << eC.message() << endl;
        #ifndef UNIX
            sleep(2);
        #else
            Sleep(2000);
        #endif
        
        this->stop();
        this->__connectToMS();
        return;
    }

    __buf_send.clear();
    __buf_send = boost::json::serialize(json_formatter::worker::request::connect(__name, __worker_id));
    __socket->async_send(net::buffer(__buf_send, __buf_send.size()), boost::bind(&Worker::__reciveConnect, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
}
void Worker::__reciveConnect(const boost::system::error_code& eC, size_t bytes_send)
{
    if (eC)
    {
        std::cerr << eC.message() << std::endl;
        std::cerr << "reciveConnect" << std::endl;
        __socket->async_send(net::buffer(__buf_send, __buf_send.size()), boost::bind(&Worker::__reciveConnect, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
        return;
    }
    static size_t temp_send = 0;
    temp_send += bytes_send;
    if (temp_send != __buf_send.size())
    {
        __log->writeTempLog(0, "DataBase", "Not_full_json");
        __socket->async_send(net::buffer(__buf_send.c_str() + temp_send, __buf_send.size() - temp_send), boost::bind(&Worker::__reciveConnect, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
        return;
    }
    temp_send = 0;
    //cerr << __buf_send << endl;
    __buf_send.clear();
    __socket->async_receive(net::buffer(__buf_recive, BUF_RECIVE_SIZE), boost::bind(&Worker::__reciveCommand, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
}
void Worker::__reciveCommand(const boost::system::error_code& eC, size_t bytes_recive)
{
    if (eC)
    {
        std::cerr << "reciveCommand " << eC.message() << std::endl;
        this->stop();
        this->__connectToMS();
        //__socket->async_receive(net::buffer(__buf_recive, BUF_RECIVE_SIZE), boost::bind(&Worker::__reciveCommand, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
        return;
    }

    size_t count_byte, count_byte_write = 0;
    for (; count_byte_write < bytes_recive;)
    {
        try
        {
            count_byte = __parser.write_some(__buf_recive + count_byte_write);
        }
        catch (std::exception& e)
        {
            std::cerr << e.what() << std::endl;
        }
        if (!__parser.done())
        {
            std::fill_n(__buf_recive, BUF_RECIVE_SIZE, 0);
            std::cerr << "Json is not full" << std::endl;
            __socket->async_receive(net::buffer(__buf_recive, BUF_RECIVE_SIZE), boost::bind(&Worker::__reciveCommand, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
            return;
        }
        try
        {
            __buf_json_recive.push(__parser.release());
            __parser.reset();
            count_byte_write += count_byte;
        }
        catch (std::exception& e)
        {
            __parser.reset();
            __buf_json_recive = {};
            std::cerr << "Json read " << e.what() << std::endl;
            __socket->async_receive(net::buffer(__buf_recive, BUF_RECIVE_SIZE), boost::bind(&Worker::__reciveCommand, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
            return;
        }
    }
    std::fill_n(__buf_recive, BUF_RECIVE_SIZE, 0);
    __buf_json_recive = {};
    __socket->async_receive(net::buffer(__buf_recive, BUF_RECIVE_SIZE), boost::bind(&Worker::__sendResponse, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
}
void Worker::__sendResponse(const boost::system::error_code& eC, size_t bytes_recive)
{
    if (eC)
    {
        std::cerr << "__sendResponse " << eC.message() << std::endl;
        this->stop();
        #ifndef UNIX
            sleep(2);
        #else
            Sleep(2000);
        #endif
        this->__connectToMS();
        //__socket->async_receive(net::buffer(__buf_recive, BUF_RECIVE_SIZE), boost::bind(&Worker::__reciveCommand, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
        return;
    }

    size_t count_byte, count_byte_write = 0;
    for (; count_byte_write < bytes_recive;)
    {
        try
        {
            count_byte = __parser.write_some(__buf_recive + count_byte_write);
        }
        catch (std::exception& e)
        {
            std::cerr << e.what() << std::endl;
        }
        if (!__parser.done())
        {
            std::fill_n(__buf_recive, BUF_RECIVE_SIZE, 0);
            std::cerr << "Json is not full" << std::endl;
            __socket->async_receive(net::buffer(__buf_recive, BUF_RECIVE_SIZE), boost::bind(&Worker::__reciveCommand, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
            return;
        }
        try
        {
            __buf_json_recive.push(__parser.release());
            __parser.reset();
            count_byte_write += count_byte;
        }
        catch (std::exception& e)
        {
            __parser.reset();
            __buf_json_recive = {};
            std::cerr << "Json read " << e.what() << std::endl;
            __socket->async_receive(net::buffer(__buf_recive, BUF_RECIVE_SIZE), boost::bind(&Worker::__reciveCommand, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
            return;
        }
    }
    std::fill_n(__buf_recive, BUF_RECIVE_SIZE, 0);

    while(__buf_json_recive.size() != 0)
    {
        try
        {
            __json_string = __buf_json_recive.front();
            __buf_json_recive.pop();

            //cerr << json_string << endl;
            boost::json::value target = __json_string.at("target");
            if (target == "ping")
            {
                __buf_send = boost::json::serialize(json_formatter::worker::response::ping(__name));
            }
            else if (target == "disconnect")
            {
                __buf_send = boost::json::serialize(json_formatter::worker::response::disconnect(__name));
                __flag_disconnect = true;
            }
            else if (target == "marussia_station_request")
            {
                __analizeRequest();
            }
            std::cerr << __flag_disconnect << std::endl;
            if (__buf_json_recive.size() == 0)
            {
                __flag_end_send = true;
            }
            __socket->async_send(net::buffer(__buf_send, __buf_send.size()), boost::bind(&Worker::__reciveAnswer, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));

        }
        catch (std::exception& e)
        {
            std::cerr << e.what() << std::endl;
            __socket->async_receive(net::buffer(__buf_recive, BUF_RECIVE_SIZE), boost::bind(&Worker::__sendResponse, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
        }
    }
}
void Worker::__reciveAnswer(const boost::system::error_code& eC, size_t bytes_send)
{
    if (eC)
    {
        std::cerr << "recieveCommand " << eC.message() << std::endl;
        __socket->async_send(net::buffer(__buf_send, __buf_send.size()), boost::bind(&Worker::__reciveAnswer, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
        return;
    }
    static size_t temp_send = 0;
    temp_send += bytes_send;
    if (temp_send != __buf_send.size())
    {
        __log->writeTempLog(0, "DataBase", "Not_full_json");
        __socket->async_send(net::buffer(__buf_send.c_str() + temp_send, __buf_send.size() - temp_send), boost::bind(&Worker::__reciveAnswer, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
        return;
    }
    temp_send = 0;
    __buf_send.clear();
    if (__flag_disconnect)
    {
        __flag_disconnect = false;
        this->stop();
    }
    else if (__flag_end_send)
    {
        __flag_end_send = false;
        __socket->async_receive(net::buffer(__buf_recive, BUF_RECIVE_SIZE), boost::bind(&Worker::__sendResponse, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
    }
    else
    {
        std::cerr << "no one send " << std::endl;
    }
}
void Worker::__connectToBd()
{
    std::queue<std::string> tables;
    tables.push("MarussiaStation"); tables.push("House"); tables.push("StaticPhrases");
    std::queue<std::vector<std::string>> fields;
    std::vector<std::string> fields_marussia = { "ApplicationId", "HouseComplexId", "HouseNum", "LiftBlockId"};
    std::vector<std::string> fields_house = { "TopFloor", "BottomFloor", "NullFloor", "HouseNumber", "HousingComplexId" };
    std::vector<std::string> fields_phrases = { "HouseComplexId", "HouseNum", "KeyWords", "Response" };
    fields.push(fields_marussia); fields.push(fields_house); fields.push(fields_phrases);

    std::queue<std::string> conditions;
    //conditions.push("WHERE WorkerId = \"1\" OR WokerSecId = \"1\""); conditions.push(""); conditions.push("");
    std::cout << __db_log << " " << __db_pas << endl;
    std::cout << __ip_db << " " << __port_db << endl;
    __db_client->setQuerys(tables, fields, conditions);
    __db_client->start();
}
void Worker::__resetTimer()
{
    __log->writeTempLog(0, __name, "__rewrite_data_base");
    __connectToBd();
    __timer->expires_from_now(boost::posix_time::hours(24));
    __timer->async_wait(boost::bind(&Worker::__resetTimer, shared_from_this()));
}

void Worker::__analizeRequest()
{
    __log->writeTempLog(0, __name, "__analize_response");
    ///___marussia station House number and complex id___///
    std::string app_id = boost::json::serialize(__json_string.at("request").at("station_id"));

    std::cerr << "app_id" << app_id << std::endl;
    std::map<std::string, std::vector<std::string>> one_table = __db_info.at("MarussiaStation");
    std::vector<std::string> buf_vec = __db_info.at("MarussiaStation").at("ApplicationId");
    std::vector<std::string> house_vec = __db_info.at("MarussiaStation").at("HouseNum");
    std::vector<std::string> comp_vec = __db_info.at("MarussiaStation").at("HouseComplexId");
    std::vector<std::string> lift_vec = __db_info.at("MarussiaStation").at("LiftBlockId");

    std::string num_house = "-1";
    std::string comp_id = "-1";
    std::string lift_block = "";
    __response_command.clear();

    for (size_t i = 0; i < buf_vec.size(); i++)
    {
        std::cout << app_id << " " << buf_vec[i] << std::endl;
        if (app_id == buf_vec[i])
        {
            std::cerr << "num_house " << num_house << std::endl << "comp_id " << comp_id << std::endl << "lift_block " << lift_block << std::endl;
            num_house = house_vec[i];
            comp_id = comp_vec[i];
            lift_block = lift_vec[i];
            break;
        }
    }
    if (num_house == "-1" || comp_id == "-1")
    {
        std::cerr << "error no House or comp" << std::endl;
        this->stop();
    }
    //cerr << "num_house " << num_house << endl;
    //cerr << "comp_id " << comp_id << endl;
    //cerr << "lift_block " << lift_block << endl;

    ///___search for mqtt_____/////
    boost::json::array array_tokens = __json_string.at("request").at("body").at("request").at("nlu").at("tokens").as_array();
    std::vector<std::string> search_mqtt;
    bool flag_mqtt = false;
    //boost::locale::generator gen;
    for (size_t i = 0; i < array_tokens.size(); i++)
    {

        std::string buf_string_mqtt = boost::json::serialize(array_tokens[i]);
        std::string string_mqtt;
        remove_copy(buf_string_mqtt.begin(), buf_string_mqtt.end(), back_inserter(string_mqtt), '"');
        search_mqtt.push_back(string_mqtt);
        for(size_t j = 0; j < __mqtt_keys.size(); j++)
        {
            //string buf_mqtt_key = boost::locale::conv::to_utf<char>(string(__mqtt_keys[j].begin(), __mqtt_keys[j].end()), gen(""));
            std::string buf_mqtt_key = __mqtt_keys[j];
            std::cerr << "array" << string_mqtt << " my" << buf_mqtt_key << std::endl;
            if (string_mqtt == buf_mqtt_key)
            {
                flag_mqtt = true;
            }
        }
    }
    std::cerr << "flag_mqtt " << flag_mqtt << std::endl;
    if (flag_mqtt)
    {
        one_table.clear();
        one_table = __db_info.at("House");
        buf_vec.clear(); house_vec.clear(); comp_vec.clear();
        house_vec = __db_info.at("House").at("HouseNumber");
        comp_vec = __db_info.at("House").at("HousingComplexId");
        std::vector<std::string> top_floor = __db_info.at("House").at("TopFloor");
        std::vector<std::string> bot_floor = __db_info.at("House").at("BottomFloor");
        std::vector<std::string> null_floor = __db_info.at("House").at("NullFloor");
        std::string bufNum = "";
        for (size_t i = 0; i < __key_roots.size(); i++)
        {
            for (size_t j = 0; j < search_mqtt.size(); j++)
            {
                //string buf_key = boost::locale::conv::to_utf<char>(string(__key_roots[i].begin(), __key_roots[i].end()), gen(""));
                std::string buf_key = __key_roots[i];
                std::cerr << search_mqtt[j] << " my" << buf_key << std::endl;
                if (search_mqtt[j].find(buf_key) != search_mqtt[j].npos)
                {
                    bufNum = search_mqtt[j];
                }
            }
        }
        std::cerr << "bufNum " << bufNum << std::endl;
        //u8string string_number;
        std::string string_number;
        remove_copy(bufNum.begin(), bufNum.end(), back_inserter(string_number), '"');
        int numFloor = __num_roots.at(string_number);
        std::cerr << numFloor  << std::endl;
        for (size_t i = 0; i < house_vec.size(); i++)
        {
            if (house_vec[i] == num_house && comp_vec[i] == comp_id)
            {
                std::string boofer;
                remove_copy(top_floor[i].begin(), top_floor[i].end(), back_inserter(boofer), '"');
                int top = stoi(boofer);

                boofer.clear();
                remove_copy(bot_floor[i].begin(), bot_floor[i].end(), back_inserter(boofer), '"');
                int bot = stoi(boofer);

                boofer.clear();
                remove_copy(null_floor[i].begin(), null_floor[i].end(), back_inserter(boofer), '"');
                int null;
                if (boofer == "-1")
                {
                    null = -1;
                }
                else
                {
                    null = 0;
                }
                //cerr << top << "" << bot << " " << null << endl;
                if (top >= numFloor && bot <= numFloor || null == numFloor)
                {
                    //__response_command = "перемещаю вас на " + numFloor + "этаж";
                    //u8string buf_u8_resp = u8"Перемещаю вас на ";
                    //__response_command = boost::locale::conv::to_utf<char>(string(buf_u8_resp.begin(), buf_u8_resp.end()), gen(""));
                    std::string buf_u8_resp = "Перемещаю вас на ";
                    __response_command = buf_u8_resp;
                    __response_command += bufNum;
                    buf_u8_resp = " этаж";
                    //__response_command += boost::locale::conv::to_utf<char>(string(buf_u8_resp.begin(), buf_u8_resp.end()), gen(""));
                    __response_command += buf_u8_resp;
                    break;
                }
            }
        }
        std::cerr << "mqtt ";
        std::cerr << __response_command << std::endl;
        __buf_send = boost::json::serialize(json_formatter::worker::response::marussia_mqtt_message(__name, app_id, __getRespToMS(__response_command), lift_block, numFloor));
        //cerr << __buf_send << endl;
        flag_mqtt = false;
    }
    else
    {
        ///_____static phrases response static phrase___///

        std::string buf_command = boost::json::serialize(__json_string.at("request").at("body").at("request").at("command"));
        std::string command;
        remove_copy(buf_command.begin(), buf_command.end(), back_inserter(command), '"');
        one_table.clear();
        one_table = __db_info.at("StaticPhrases");
        buf_vec.clear(); house_vec.clear(); comp_vec.clear();
        buf_vec = __db_info.at("StaticPhrases").at("KeyWords");
        house_vec = __db_info.at("StaticPhrases").at("HouseNum");
        comp_vec = __db_info.at("StaticPhrases").at("HouseComplexId");
        std::vector<std::string> resp = __db_info.at("StaticPhrases").at("Response");
        bool flag_stop_search = false;

        for (size_t i = 0; i < buf_vec.size(); i++)
        {
            std::string all_variants = buf_vec[i];
            std::vector<std::string> vector_variants;
            int num = all_variants.find(";");
            if (num != all_variants.npos)
            {
                while (num != all_variants.npos)
                {
                    std::string buf_string;
                    buf_string.assign(all_variants, 0, num);
                    all_variants.erase(0, buf_string.size() + 1);
                    vector_variants.push_back(buf_string);
                    num = all_variants.find(";");
                }
                vector_variants.push_back(all_variants);
            }
            else
            {
                vector_variants.push_back(all_variants);
            }
            for(size_t j = 0; j < vector_variants.size(); j++)
            {
                std::string buf_variant;
                remove_copy(vector_variants[j].begin(), vector_variants[j].end(), back_inserter(buf_variant), '"');
                std::cerr << command << " " << buf_variant << std::endl;
                if (command.find(buf_variant)!= string::npos)
                {
                    std::cerr << command << " " << buf_variant << std::endl;
                    if (num_house == house_vec[i] && comp_id == comp_vec[i])
                    {
                        std::cerr << "resp " << resp[i] << std::endl;
                        __response_command = resp[i];
                        flag_stop_search = true;
                        break;
                    }
                }
            }
            if (flag_stop_search)
            {
                flag_stop_search = false;
                break;
            }
        }
        if (!__response_command.empty())
        {
            __buf_send = boost::json::serialize(json_formatter::worker::response::marussia_static_message(__name, app_id, __getRespToMS(__response_command)));
            std::cerr << __buf_send << std::endl;
        }
        else
        {
            std::string buf_resp_u8 = "Извините, я не знаю такой команды, пожалуйста, перефразируйте";
            __response_command.clear();
            __response_command = buf_resp_u8;
            //__response_command = boost::locale::conv::to_utf<char>(string(buf_resp_u8.begin(), buf_resp_u8.end()), gen(""));
            __buf_send = boost::json::serialize(json_formatter::worker::response::marussia_static_message(__name, app_id, __getRespToMS(__response_command)));
            std::cerr << __buf_send << std::endl;
        }
    }

}
boost::json::object Worker::__getRespToMS(string respText)
{
    return boost::json::object({
                                {"text", respText},
                                {"tts", respText},
                                {"end_session", false}
                                });
}