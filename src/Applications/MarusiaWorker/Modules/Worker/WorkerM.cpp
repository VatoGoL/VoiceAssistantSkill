#include "WorkerM.hpp"

void WorkerM::__сallback(std::map<std::string, std::map<std::string, std::vector<std::string>>> data)
{
    setDbInfo(data);
}
WorkerM::WorkerM(net::io_context& ioc):
                __ioc(ioc),
                callback(boost::bind(&WorkerM::__сallback, this, boost::placeholders::_1))
{
    __socket = std::make_shared<tcp::socket>(__ioc);
    __buf_recive = new char[BUF_RECIVE_SIZE + 1];
    __buf_send = "";
    __buf_json_recive = {};
    __parser.reset();
    __timer = std::make_shared<net::deadline_timer>(__ioc);
    __flag_disconnect = false;
    __text_presentation = "Текст презентации университета не был загружен";
	__text_opportunities = "Текст возможностей скила не был загружен";
}
WorkerM::PROCESS_CODE WorkerM::init()
{
    std::map<std::string, std::string>& configuration = Configer::getConfigInfo();
    Logger::writeLog("WorkerM","init",Logger::log_message_t::EVENT, "WorkerM init starting");

    try
    {
        char buffer[1025];
        std::string file_text = "";
        std::ifstream fin(configuration.at("Presentation_text"));
        std::fill_n(buffer, 1024,0);
        while(fin.readsome(buffer, 1024) != 0){
            file_text += buffer;
            std::fill_n(buffer, 1024,0);
        }
        fin.close();

        __parser.write(file_text);
        __text_presentation = __parser.release().as_string();
        __parser.reset();

        file_text = "";
        fin.open(configuration.at("Opportunities_text"));
	    std::fill_n(buffer, 1024,0);
        while(fin.readsome(buffer, 1024) != 0){
            file_text += buffer;
            std::fill_n(buffer, 1024,0);
        }
        fin.close();
        
        __parser.write(file_text);
        __text_opportunities = __parser.release().as_string();
        __parser.reset();

        __ip_ms = configuration.at("Main_server_ip");
        __port_ms = std::stoi(configuration.at("Main_server_port"));
        __worker_id = configuration.at("Id");
        __ip_db = configuration.at("BD_ip");
        __port_db = std::stoi(configuration.at("BD_port"));
        __db_log = configuration.at("BD_login");
        __db_pas = configuration.at("BD_password");
        __socket_dB = std::make_shared<tcp::socket>(__ioc);
        __db_client = std::make_shared<ClientDB>(__ip_db, __port_db, __db_log, __db_pas, __name, __socket_dB, callback);
        __end_point = std::make_shared<tcp::endpoint>(tcp::endpoint(net::ip::address::from_string(__ip_ms), __port_ms));
        if (__port_ms < 1 || __port_db < 1) 
        {
            Logger::writeLog("WorkerM","init",Logger::log_message_t::ERROR,"Port <= 0");
            return PROCESS_CODE::CONFIG_DATA_NOT_CORRECT;
        }
    }
    catch (std::exception& e)
    {
        Logger::writeLog("WorkerM","init",Logger::log_message_t::ERROR,e.what());
        this->stop();
        return PROCESS_CODE::CONFIG_DATA_NOT_FULL;
    }

    Logger::writeLog("WorkerM","init",Logger::log_message_t::EVENT, "WorkerM init complete");
    return PROCESS_CODE::SUCCESSFUL;
}
WorkerM::~WorkerM()
{
    this->stop();
    delete[] __buf_recive;
}
void WorkerM::start()
{
    Logger::writeLog("WorkerM","start",Logger::log_message_t::EVENT, "WorkerM start");
    __timer->expires_from_now(boost::posix_time::hours(24));
    __timer->async_wait(boost::bind(&WorkerM::__resetTimer, shared_from_this()));
    
    __connectToDB();
    __connectToMS();
}
void WorkerM::stop()
{
    if (__socket->is_open())
    {
        __socket->close();
    }
    Logger::writeLog("WorkerM","start",Logger::log_message_t::EVENT, "WorkerM stop");
}
void WorkerM::setDbInfo(std::map <std::string, std::map<std::string, std::vector<std::string>>> data)
{
    std::map<std::string, std::vector<std::string>> temp_table_direction_of_preparation;
    std::map<std::string, std::vector<std::string>> temp_table_interesting_fact;
    std::map<std::string, std::vector<std::string>> temp_table_static_phrases;
    std::map<std::string, std::vector<std::string>> temp_table_professors_name;
    std::map<std::string, std::vector<std::string>> temp_table_groups_name;
    std::map<std::string, std::vector<std::string>> temp_table_university_fact;
    std::map<std::string, std::vector<std::string>> temp_table_days_week;
    std::map<std::string, std::vector<std::string>> temp_table_directions;
    try{
        temp_table_direction_of_preparation = data["DirectionOfPreparation"];
        temp_table_interesting_fact = data["InterestingFact"];
        temp_table_static_phrases = data["StaticPhrases"];
        temp_table_university_fact = data["UniversityFact"];
        temp_table_professors_name = data["ProfessorsName"];
        temp_table_groups_name = data["GroupsName"];
        temp_table_days_week = data["DaysWeek"];
        temp_table_directions = data["Directions"];
    }catch(std::exception &e){
        Logger::writeLog("WorkerM","setDbInfo",Logger::log_message_t::WARNING, "The data from DB is not correct");
        return;
    }
    __table_direction_of_preparation = temp_table_direction_of_preparation;
    __table_interesting_fact = temp_table_interesting_fact;
    __table_static_phrases = temp_table_static_phrases;
    __table_university_fact = temp_table_university_fact;
    __table_groups_name = temp_table_groups_name;
    __table_professors_name = temp_table_professors_name;
    __table_days_week = temp_table_days_week;
    __table_directions = temp_table_directions;
    
    
    __parseKeyWords(__vectors_variants,__table_static_phrases, "key_words", "response");
    __parseKeyWords(__vectors_variants_professors,__table_professors_name, "key_words", "response");
    __parseKeyWords(__vectors_variants_groups,__table_groups_name, "key_words", "response");
    __parseKeyWords(__vectors_days_week,__table_days_week, "key_words", "response");
    __parseKeyWords(__vectors_directions,__table_directions, "key_words", "response");
}

void WorkerM::__parseKeyWords(std::vector<std::pair<std::vector<std::string>, std::string >> &vectors_variants,
                     const std::map<std::string, std::vector<std::string>>& table,
					 const std::string& key_words_name_field, const std::string& response_name_field)
{
    vectors_variants.clear();
    std::vector<std::string> vector_variants;
    std::string all_variants;
    std::string buf_string;

    for(auto key = table.at(key_words_name_field).begin(), end_key = table.at(key_words_name_field).end(),
             value = table.at(response_name_field).begin(), end_value = table.at(response_name_field).end(); 
             key != end_key; 
             key++, value++
        )
    {
        all_variants.clear();
        remove_copy(key->begin(), key->end(), back_inserter(all_variants), '"');
        //all_variants = (*key);
        vector_variants.clear();
        int num = all_variants.find(";");
        if (num != all_variants.npos)
        {
            while (num != all_variants.npos)
            {
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
        vectors_variants.push_back(std::make_pair(vector_variants,*value));
    }
}

void WorkerM::__connectToMS()
{
    __socket->async_connect(*__end_point, boost::bind(&WorkerM::__sendConnect, this, boost::placeholders::_1));
}
void WorkerM::__sendConnect(const boost::system::error_code& eC)
{
    if (eC)
    {
        Logger::writeLog("WorkerM","__sendConnect",Logger::log_message_t::ERROR, eC.message());
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
    __socket->async_send(net::buffer(__buf_send, __buf_send.size()), boost::bind(&WorkerM::__reciveConnect, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
}
void WorkerM::__reciveConnect(const boost::system::error_code& eC, size_t bytes_send)
{
    if (eC)
    {
        Logger::writeLog("WorkerM","__reciveConnect",Logger::log_message_t::ERROR, eC.message());
        __socket->async_send(net::buffer(__buf_send, __buf_send.size()), boost::bind(&WorkerM::__reciveConnect, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
        return;
    }
    static size_t temp_send = 0;
    temp_send += bytes_send;
    if (temp_send != __buf_send.size())
    {
        //ненужный лог (скорее всего)
        Logger::writeLog("WorkerM","__reciveConnect",Logger::log_message_t::ERROR, "DataBase not full json");
        __socket->async_send(net::buffer(__buf_send.c_str() + temp_send, __buf_send.size() - temp_send), boost::bind(&WorkerM::__reciveConnect, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
        return;
    }
    temp_send = 0;
    __buf_send.clear();
    __socket->async_receive(net::buffer(__buf_recive, BUF_RECIVE_SIZE), boost::bind(&WorkerM::__reciveCommand, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
}
void WorkerM::__reciveCommand(const boost::system::error_code& eC, size_t bytes_recive)
{
    if (eC)
    {
        Logger::writeLog("WorkerM","__reciveCommand",Logger::log_message_t::ERROR, eC.message() );
        this->stop();
        this->__connectToMS();
        //__socket->async_receive(net::buffer(__buf_recive, BUF_RECIVE_SIZE), boost::bind(&WorkerM::__reciveCommand, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
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
            Logger::writeLog("WorkerM","__reciveCommand",Logger::log_message_t::ERROR, "Json is not full");
            __socket->async_receive(net::buffer(__buf_recive, BUF_RECIVE_SIZE), boost::bind(&WorkerM::__reciveCommand, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
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
            Logger::writeLog("WorkerM","__reciveCommand",Logger::log_message_t::ERROR, std::string("Json read") + e.what());
            __socket->async_receive(net::buffer(__buf_recive, BUF_RECIVE_SIZE), boost::bind(&WorkerM::__reciveCommand, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
            return;
        }
    }
    std::fill_n(__buf_recive, BUF_RECIVE_SIZE, 0);
    __buf_json_recive = {};
    __socket->async_receive(net::buffer(__buf_recive, BUF_RECIVE_SIZE), boost::bind(&WorkerM::__sendResponse, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
}
void WorkerM::__sendResponse(const boost::system::error_code& eC, size_t bytes_recive)
{
    if (eC)
    {
        Logger::writeLog("WorkerM","__sendResponse",Logger::log_message_t::ERROR, eC.message());
        this->stop();
        #ifndef UNIX
            sleep(2);
        #else
            Sleep(2000);
        #endif
        this->__connectToMS();
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
            Logger::writeLog("WorkerM","__sendResponse",Logger::log_message_t::ERROR, "Json not full");
            __socket->async_receive(net::buffer(__buf_recive, BUF_RECIVE_SIZE), boost::bind(&WorkerM::__reciveCommand, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
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
            Logger::writeLog("WorkerM","__sendResponse",Logger::log_message_t::ERROR, std::string("Json read") + e.what());
            __socket->async_receive(net::buffer(__buf_recive, BUF_RECIVE_SIZE), boost::bind(&WorkerM::__reciveCommand, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
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

            boost::json::value target = __json_string.at("target");
            if (target == "ping")
            {
                __buf_send = boost::json::serialize(json_formatter::worker::response::ping(__name));
            }
            else if(target == "load_check"){
                //!!! вернуть данные о загруженности машины
                 __buf_send = boost::json::serialize(json_formatter::worker::response::checkSystemStat
                                                    (__name, SystemStat::busyCPU(), SystemStat::busyMemory()));
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
            __socket->async_send(net::buffer(__buf_send, __buf_send.size()), boost::bind(&WorkerM::__reciveAnswer, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));

        }
        catch (std::exception& e)
        {
            std::cerr << e.what() << std::endl;
            __socket->async_receive(net::buffer(__buf_recive, BUF_RECIVE_SIZE), boost::bind(&WorkerM::__sendResponse, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
        }
    }
}
void WorkerM::__reciveAnswer(const boost::system::error_code& eC, size_t bytes_send)
{
    if (eC)
    {
        Logger::writeLog("WorkerM","__reciveAnswer",Logger::log_message_t::ERROR, std::string("Json read") + eC.message());
        __socket->async_send(net::buffer(__buf_send, __buf_send.size()), boost::bind(&WorkerM::__reciveAnswer, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
        return;
    }
    static size_t temp_send = 0;
    temp_send += bytes_send;
    if (temp_send != __buf_send.size())
    {
        Logger::writeLog("WorkerM","__reciveAnswer",Logger::log_message_t::ERROR, "DataBase not full json");
        __socket->async_send(net::buffer(__buf_send.c_str() + temp_send, __buf_send.size() - temp_send), boost::bind(&WorkerM::__reciveAnswer, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
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
        __socket->async_receive(net::buffer(__buf_recive, BUF_RECIVE_SIZE), boost::bind(&WorkerM::__sendResponse, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
    }
    else
    {
        Logger::writeLog("WorkerM","__reciveAnswer",Logger::log_message_t::ERROR, "no one send");
    }
}
void WorkerM::__connectToDB()
{
    std::queue<std::string> tables;
    tables.push("DirectionOfPreparation"),tables.push("InterestingFact"),tables.push("UniversityFact"),
    tables.push("StaticPhrases"), tables.push("GroupsName"),tables.push("ProfessorsName"), tables.push("DaysWeek"),
    tables.push("Directions");
   
    std::vector<std::string> fields_direction_of_preparation = { "direction", "number_of_budget_positions", "minimum_score"};
    std::vector<std::string> fields_interesting_fact = {"fact"};
    std::vector<std::string> fields_university_fact = {"fact"};
    std::vector<std::string> fields_static_phrases = {"key_words", "response"};
    std::vector<std::string> fields_groups_name = {"key_words", "response"};
    std::vector<std::string> fields_professors_name = {"key_words", "response"};
    std::vector<std::string> fields_days_week = {"key_words", "response"};
    std::vector<std::string> fields_directions = {"key_words", "response"};

    std::queue<std::vector<std::string>> fields;
    fields.push(fields_direction_of_preparation), fields.push(fields_interesting_fact),
    fields.push(fields_university_fact), fields.push(fields_static_phrases), 
    fields.push(fields_groups_name),fields.push(fields_professors_name),
    fields.push(fields_days_week), fields.push(fields_directions);

    std::queue<std::string> conditions;
    
    __db_client->setQuerys(tables, fields, conditions);
    __db_client->start();
}
void WorkerM::__resetTimer()
{
    Logger::writeLog("WorkerM","__resetTimer",Logger::log_message_t::ERROR, "__rewrite_data_base");
    __connectToDB();
    __timer->expires_from_now(boost::posix_time::hours(24));
    __timer->async_wait(boost::bind(&WorkerM::__resetTimer, shared_from_this()));
}
void WorkerM::__dialogSessionsStep(const std::string &app_id, const std::string& command)
{
    auto& session_step = __active_dialog_sessions[app_id];
    
    if(session_step.first == "number_of_places"){
        std::string direction, result;
        switch(session_step.second){
            case 1:
                direction = __findVariant(__vectors_directions,command);
                if(direction.empty()){
                    __buf_send = boost::json::serialize(json_formatter::worker::response::marussia_static_message(__name, app_id, 
                                                        __getRespToMS( "Извините я не смогла найти выбранное Вами направление. Попробуем ещё?") ));
                    break;
                }
                result = __findDataInTable(__table_direction_of_preparation,"direction",direction,"number_of_budget_posiitons");
                if(result.empty()){
                    __buf_send = boost::json::serialize(json_formatter::worker::response::marussia_static_message(__name, app_id, 
                                                        __getRespToMS( "Неизвестная ошибка, попробуйте другие возможности скила") ));
                    session_step.second = 0;
                    break;
                }
                __buf_send = boost::json::serialize(json_formatter::worker::response::marussia_static_message(__name, app_id, 
                                                        __getRespToMS( "В прошлом году было " + result + " бюджетных мест. Рассказать ещё?") ));
                session_step.second++;
                break;
                
            break;
            case 2:
                if(command.find("Да") != std::string::npos || command.find("да") != std::string::npos)
                {
                    __buf_send = boost::json::serialize(json_formatter::worker::response::marussia_static_message(__name, app_id, 
                                                        __getRespToMS(  "Какой номер направления Вас интересует?" ) ));
                    session_step.second--; 
                }else{
                    __buf_send = boost::json::serialize(json_formatter::worker::response::marussia_static_message(__name, app_id, __getRespToMS( "Чем я ещё могу Вам помочь?") ));
                    session_step.second = 0;
                }
            break;
        }
    }else if(session_step.first == "min_score"){
        std::string direction, result;
        switch(session_step.second){
            case 1:
                direction = __findVariant(__vectors_directions,command);
                if(direction.empty()){
                    __buf_send = boost::json::serialize(json_formatter::worker::response::marussia_static_message(__name, app_id, 
                                                        __getRespToMS( "Извините я не смогла найти выбранное Вами направление. Попробуем ещё?") ));
                    break;
                }
                result = __findDataInTable(__table_direction_of_preparation,"direction",direction,"minimum_score");
                if(result.empty()){
                    __buf_send = boost::json::serialize(json_formatter::worker::response::marussia_static_message(__name, app_id, 
                                                        __getRespToMS( "Неизвестная ошибка, попробуйте другие возможности скила") ));
                    session_step.second = 0;
                    break;
                }
                __buf_send = boost::json::serialize(json_formatter::worker::response::marussia_static_message(__name, app_id, 
                                                        __getRespToMS( "В прошлом году проходной балл был равен " + result + ". Рассказать ещё?") ));
                session_step.second++;
                break;
            break;
            case 2:
                if(command.find("Да") != std::string::npos || command.find("да") != std::string::npos)
                {
                    __buf_send = boost::json::serialize(json_formatter::worker::response::marussia_static_message(__name, app_id, 
                                                        __getRespToMS(  "Какой номер направления Вас интересует?" ) ));
                    session_step.second--;
                }else{
                    __buf_send = boost::json::serialize(json_formatter::worker::response::marussia_static_message(__name, app_id, __getRespToMS( "Чем я ещё могу Вам помочь?") ));
                    session_step.second = 0;
                }
            break;
        }
    }else if(session_step.first == "university_fact"){
        switch(session_step.second){
            case 1:

                if(command.find("Да") != std::string::npos || command.find("да") != std::string::npos)
                {
                    if(__table_university_fact.at("fact").size() != 0){
                        __buf_send = boost::json::serialize(json_formatter::worker::response::marussia_static_message(__name, app_id, 
                                                        __getRespToMS( __table_university_fact.at("fact") [rand()%__table_university_fact.at("fact").size()]  + ". Рассказать ещё?") ));
                    }else{
                        __buf_send = boost::json::serialize(json_formatter::worker::response::marussia_static_message(__name, app_id, __getRespToMS( "Извините, что-то я все подзабыла") ));
                        session_step.second = 0;
                    }
                }else{
                    __buf_send = boost::json::serialize(json_formatter::worker::response::marussia_static_message(__name, app_id, __getRespToMS( "Чем я ещё могу Вам помочь?") ));
                    session_step.second = 0;
                }
            break;
        }
    }else if(session_step.first == "interesting_fact"){
        switch(session_step.second){
            case 1:

                if(command.find("Да") != std::string::npos || command.find("да") != std::string::npos)
                {
                    if(__table_university_fact.at("fact").size() != 0){
                        __buf_send = boost::json::serialize(json_formatter::worker::response::marussia_static_message(__name, app_id, 
                                                        __getRespToMS( __table_interesting_fact.at("fact") [rand()%__table_interesting_fact.at("fact").size()]  + ". Рассказать ещё?") ));
                    }else{
                        __buf_send = boost::json::serialize(json_formatter::worker::response::marussia_static_message(__name, app_id, __getRespToMS( "Извините, что-то я все подзабыла") ));
                        session_step.second = 0;
                    }
                }else{
                    __buf_send = boost::json::serialize(json_formatter::worker::response::marussia_static_message(__name, app_id, __getRespToMS( "Чем я ещё могу Вам помочь?") ));
                    session_step.second = 0;
                }
            break;
        }
    }
}
void WorkerM::__clearZombieSessions()
{
    for(auto i = __active_dialog_sessions.begin(), end_i = __active_dialog_sessions.end(); i != end_i; i++)
    {
        if(i->second.second == 0){
            __active_dialog_sessions.erase(i);
            return __clearZombieSessions();
        }
    }
}
void WorkerM::__analizeRequest()
{ 
    __clearZombieSessions();
    std::string app_id = boost::json::serialize(__json_string.at("request").at("station_id"));
    std::string buf_command = boost::json::serialize(__json_string.at("request").at("body").at("request").at("command"));
    std::string command;
    remove_copy(buf_command.begin(), buf_command.end(), back_inserter(command), '"');

    for(auto i = __active_dialog_sessions.begin(), end_i = __active_dialog_sessions.end(); i != end_i; i++)
    {
        if(app_id == i->first){
            __dialogSessionsStep(app_id, command);
            return;
        }
    }

    __response_command = __findVariant(__vectors_variants,command);

    if (!__response_command.empty())
    {
        //__buf_send = boost::json::serialize(json_formatter::worker::response::marussia_static_message(__name, app_id, __getRespToMS(__response_command)));
        __responseTypeAnalize(__response_command, app_id, command);
    }
    else
    {
        __response_command.clear();
        __response_command = "Извините, я не знаю такой команды, пожалуйста, перефразируйте";
        __buf_send = boost::json::serialize(json_formatter::worker::response::marussia_static_message(__name, app_id, __getRespToMS(__response_command)));
    }
}
boost::json::object WorkerM::__getRespToMS(const std::string& response_text)
{
    return boost::json::object({
                                {"text", response_text},
                                {"tts", response_text},
                                {"end_session", false}
                                });
}
void WorkerM::__responseTypeAnalize(const std::string &response_type, const std::string& app_id, const std::string& command)
{
    if(response_type == "presentation"){
        __buf_send = boost::json::serialize(json_formatter::worker::response::marussia_static_message(__name, app_id, __getRespToMS(__text_presentation)));
    }else if(response_type == "opportunities"){
        __buf_send = boost::json::serialize(json_formatter::worker::response::marussia_static_message(__name, app_id, __getRespToMS(__text_opportunities)));
    }
    else if(response_type == "university_fact"){
        if(__table_university_fact.at("fact").size() != 0){
            __active_dialog_sessions[app_id] = std::make_pair(response_type, 1);
            __buf_send = boost::json::serialize(json_formatter::worker::response::marussia_static_message(__name, app_id, 
                                            __getRespToMS( __table_university_fact.at("fact") [rand()%__table_university_fact.at("fact").size()]  + ". Рассказать ещё?") ));
        }else{
            __buf_send = boost::json::serialize(json_formatter::worker::response::marussia_static_message(__name, app_id, __getRespToMS( "Извините, что-то я все подзабыла") ));
        }
    }
    else if(response_type == "interesting_fact"){
        if(__table_interesting_fact.at("fact").size() != 0){
            __active_dialog_sessions[app_id] = std::make_pair(response_type, 1);
            __buf_send = boost::json::serialize(json_formatter::worker::response::marussia_static_message(__name, app_id, 
                                            __getRespToMS( __table_interesting_fact.at("fact") [rand()%__table_interesting_fact.at("fact").size()]   + ". Рассказать ещё?") ));
        }else{
            __buf_send = boost::json::serialize(json_formatter::worker::response::marussia_static_message(__name, app_id, __getRespToMS( "Извините, что-то я все подзабыла") ));
        }
    }
    else if(response_type == "number_of_places"){
        __active_dialog_sessions[app_id] = std::make_pair(response_type, 1);
        __buf_send = boost::json::serialize(json_formatter::worker::response::marussia_static_message(__name, app_id, 
                                            __getRespToMS( "Какой номер направления Вас интересует?" ) ));
    }
    else if(response_type == "min_score"){
        __active_dialog_sessions[app_id] = std::make_pair(response_type, 1);
        __buf_send = boost::json::serialize(json_formatter::worker::response::marussia_static_message(__name, app_id, 
                                            __getRespToMS( "Какой номер направления Вас интересует?" ) ));
    }
    else if(response_type == "group_classes_now" || response_type == "professor_classes_now" ||
            response_type == "group_classes_future" || response_type == "professor_classes_future")
    {
        __buf_send = __findSchedule(app_id, __vectors_variants_groups, command);
    }
}

std::string WorkerM::__findVariant(const std::vector<std::pair<std::vector<std::string>, std::string >>& variants, const std::string& data)
{
    for(auto i = variants.begin(), end_i = variants.end(); i != end_i; i++)
    {
        for(auto j = i->first.begin(), end_j = i->first.end(); j != end_j; j++)
        {
            if(data.find(*j) != std::string::npos)
            {
                return i->second;
            }
        }
    }
    return "";
}
std::string WorkerM::__findSchedule(const std::string& app_id, const std::vector<std::pair<std::vector<std::string>, std::string >>& variants_target, const std::string& command)
{
    std::string target, day;
    target = __findVariant(variants_target, command);
    if(target.empty()){
        return boost::json::serialize(json_formatter::worker::response::marussia_static_message(__name, app_id, 
                                        __getRespToMS( "Названной вами группы не существует." )));    
    }
    day = __findVariant(__vectors_days_week, command);
    if(day.empty()){
        return boost::json::serialize(json_formatter::worker::response::marussia_static_message(__name, app_id, 
                                        __getRespToMS( "День недели не найден. Вы уверены что назвали его правильно?" )));    
    }
    return boost::json::serialize(json_formatter::worker::response::marussia_static_message(__name, app_id, 
                                        __getRespToMS( __schedule_manager.getSchedule(target, day) + ". Чем я ещё могу Вам помочь?") ));
}

std::string WorkerM::__findDataInTable(const std::map<std::string, std::vector<std::string>>& table, 
                                       const std::string& target_field, const std::string& target_value,
                                       const std::string& data_field)
{
    try{
        size_t position = 0;
        for(auto i = table.at(target_field).begin(), end_i = table.at(target_field).end(); i != end_i; i++)
        {
            if(target_value == *i){
                return table.at(data_field)[position];
            }
            position++;
        }
    }catch(std::exception& e){
        Logger::writeLog("WorkerM", "__findDataInTable", Logger::log_message_t::ERROR, "Поиск данных в таблице, target_filed: " + target_field + 
                                                                                        ", target_value: " + target_value + ", data_field: " + data_field);
        return "";
    }
    return "";
}