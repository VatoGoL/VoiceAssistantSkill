#include "SystemStat.hpp"
long SystemStat::busyMemory()
{
    std::ifstream fin("/proc/meminfo");
    if(!fin.is_open()){
        Logger::writeLog("SystemStat","busyMemory",Logger::log_message_t::ERROR, "file /proc/meminfo not open");
        return 100;
    }
    unsigned long total_memory;
    unsigned long available_memory;

    std::string field;
    
    do{
        fin >> field;
    }while(field != "MemTotal:");
    fin >> total_memory;

    do{
        fin >> field;
    }while(field != "MemAvailable:");
    fin >> available_memory;
    fin.close();
    return static_cast<long>(static_cast<double>(total_memory - available_memory) / static_cast<double>(total_memory) * 100.0);
}
long SystemStat::busyCPU()
{
    auto first = __getActualCPU();
    sleep(1);
    auto second = __getActualCPU();
    double total_over = static_cast<double>(second.first) - static_cast<double>(first.first);
    double work_over = static_cast<double>(second.second) - static_cast<double>(first.second);
    return static_cast<long>((1.0 - (work_over / total_over)) * 100.0);
}
std::pair<unsigned long,unsigned long> SystemStat::__getActualCPU(){
    std::ifstream fin("/proc/stat");
    if(!fin.is_open()){
        Logger::writeLog("SystemStat","busyCPU",Logger::log_message_t::ERROR, "file /proc/meminfo not open");
        return std::make_pair(0,0);
    }
    std::string field;
    static const uint8_t COUNT_FIELD = 7;
    std::vector<unsigned long> data(COUNT_FIELD); // 7 - кол-во заполненых ячеек данных
    unsigned long total = 0, first_three = 0;
    do{
        fin >> field;
    }while(field != "cpu");

    for(uint8_t i = 0; i < COUNT_FIELD; i++){
        fin >> data[i];
        total += data[i];
    }
    //user + nice + system
    first_three = data[0] + data[1] + data[2];
    fin.close();
    return std::make_pair(total,first_three);
}