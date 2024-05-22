#include "ScheduleManager.hpp"
const std::vector<std::string> ScheduleManager::__DAYS = {"monday", "tuesday", "wednesday", "thuersday", "friday", "saturday"};
void ScheduleManager::init(const std::string& path_to_schedule)
{
    std::ifstream fin(path_to_schedule);
    if(!fin.is_open()){
        throw std::invalid_argument("path_to_schedule not open, path: " + path_to_schedule);
    }
    char buffer[1025];
    boost::json::stream_parser parser;
    //parser.reset();
    
    std::fill_n(buffer, 1024,0);
    size_t count_read = 0;
    while((count_read = fin.readsome(buffer, 1024)) != 0){
        parser.write_some(buffer, count_read);
        std::fill_n(buffer, 1024,0);
    }
    fin.close();
    
    if(!parser.done()){
        std::cerr << "parser not done" << std::endl;

    }else{
        __schedule_data = parser.release();
        std::cerr <<boost::json::serialize(__schedule_data) << std::endl;
    }
    
    
    parser.reset();
}
std::string ScheduleManager::getSchedule(const std::string& target, const std::string& day, const bool& is_first_week, const bool& is_professors)
{
    std::string schedule_result;
    try{
        const boost::json::array& target_arr = is_professors ? __schedule_data.at("professors").get_array() : __schedule_data.at("students").get_array();
        boost::json::value week;
        for(auto i = target_arr.begin(),end_i = target_arr.end(); i != end_i; i++)
        {
            if(i->at("name").get_string() == target){
                if(is_first_week){
                    week = i->at("classes").at("first");
                }else{
                    week = i->at("classes").at("second");
                }
                break;
            }
        }
        boost::json::array temp_array;
        for(auto i = __DAYS.begin(), end_i = __DAYS.end(); i != end_i; i++)
        {
            temp_array = week.at(*i).get_array();
            for(auto j = temp_array.begin(), end_j = temp_array.end(); j != end_j; j++)
            {
                schedule_result += "В " + std::string(j->at("time").get_string()) + " " +  std::string(j->at("lesson_name").get_string()) + 
                                    " (" + std::string(j->at("lesson_type").get_string()) + ")" + 
                                    " в аудитории " + std::string(j->at("office").get_string()) + ", корпус " + std::string(j->at("building").get_string()) + ". ";
            }
        }

    }catch(std::exception& e){
        schedule_result = "Извините пожалуйста, но расписание куда-то запропостилось";
    }
    return schedule_result;
}