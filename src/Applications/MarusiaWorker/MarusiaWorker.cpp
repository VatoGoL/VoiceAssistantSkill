#include "MarusiaWorker.hpp"

MarusiaWorker::MarusiaWorker(string way_conf_file, string name_conf_file)
{
    __log_server = std::make_shared<Logger>("Log/", "./", "Marussia_Worker");
    std::string way = "";
    if (name_conf_file == "")
    {
        name_config = DEFINE_CONFIG;
    }
    else
    {
        name_config = name_conf_file;
    }
    if (way_conf_file != "")
    {
        way = way_conf_file;
    }
    std::cerr << way << std::endl;
    __config = std::make_shared<Configer>(__log_server, "./", way, name_config);
    __config->readConfig();
    __config_info = __config->getConfigInfo();
}
void MarusiaWorker::run()
{
    if (__config_info.size() == 0)
    {
        __log_server->writeLog(1, "Marussia_Worker", "Config_File_not_open");
        return;
    }
    else if (__config_info.size() < CONFIG_NUM_FIELDS)
    {
        __log_server->writeLog(1, "Marussia_Worker", "Config_File_not_full");
        return;
    }
    try
    {
        for (size_t i = 0, length = __config_info.size() - 1; i < length; i++)
        {
            std::cerr << CONFIG_FIELDS.at(i) << std::endl;
            __config_info.at(CONFIG_FIELDS.at(i));
        }
    }
    catch (std::exception& e)
    {
        __log_server->writeLog(1, "Marussia_Worker", e.what());
        return;
    }
    try
    {
        __count_threads = stoi(__config_info.at("Count_threads"));
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
    __ioc = make_shared<boost::asio::io_context>(__count_threads);
    __session = make_shared<Worker>(__config_info, *__ioc, __log_server);
    __session->start();
    std::vector<std::thread> v;
    v.reserve(__count_threads - 1);
    for (auto i = __count_threads - 1; i > 0; --i)
        v.emplace_back(
            [this]
            {
                __ioc->run();
            });
    __ioc->run();
}
void MarusiaWorker::stop()
{
    __session->stop();
}
MarusiaWorker::~MarusiaWorker()
{
    this->stop();
}