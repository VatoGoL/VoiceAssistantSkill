#pragma once
#include <string>
#include <fstream>
#include <vector>
#include <boost/json.hpp>
#include <GlobalModules/Logger/Logger.hpp>
class ScheduleManager{
public:
    void init(const std::string& path_to_schedule);
    std::string getSchedule(const std::string& target, const std::string& day, const bool& is_first_week, const bool& is_professors);
private:
    boost::json::value __schedule_data;
    static const std::vector<std::string> __DAYS;
};