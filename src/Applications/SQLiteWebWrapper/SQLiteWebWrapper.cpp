#include "SQLiteWebWrapper.hpp"

ServerDataBase::ServerDataBase(std::string config_way, std::string name_conf)
{
    std::string way = "";
    if (name_conf == "")
    {
        __name_config = DEFINE_CONFIG;
    }
    else
    {
        __name_config = name_conf;
    }
    if (config_way != "")
    {
        way = config_way;
    }
    __log_server = std::make_shared<Logger>("Log/", "./", "DataBase");
    __config = std::make_shared<Configer>(__log_server, "./", way, __name_config);
    __config->readConfig();
}
ServerDataBase::~ServerDataBase()
{
    stop();
}
void ServerDataBase::start()
{
    __config_info = __config->getConfigInfo();
    if (__config_info.size() == 0)
    {
        __log_server->writeLog(1, "DataBase", "Config_File_not_open");
        return;
    }
    else if (__config_info.size() < CONFIG_NUM_FIELDS)
    {
        __log_server->writeLog(1, "DataBase", "Config_File_not_full");
    }
    try
    {
        for (size_t i = 0, length = __config_info.size() - 1; i < length; i++)
        {
            __config_info.at(CONFIG_FIELDS.at(i));
            std::cerr << CONFIG_FIELDS.at(i) << " " << __config_info.at(CONFIG_FIELDS.at(i)) << std::endl;
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "error" << std::endl;
        __log_server->writeLog(1, "DataBase", e.what());
        return;
    }
    __countThreads = std::stoi(__config_info.at("Count_threads"));
    __ioc = std::make_shared<boost::asio::io_context>(__countThreads);
    __server = std::make_shared<Server>(__ioc, "", __log_server, __config_info);
    __server->run();

    std::vector<std::thread> v;
    v.reserve(__countThreads - 1);
    for (auto i = __countThreads - 1; i > 0; --i)
        v.emplace_back(
            [this]
            {
                __ioc->run();
            });
    __ioc->run();
}
void ServerDataBase::stop()
{
    __server->stop();
}