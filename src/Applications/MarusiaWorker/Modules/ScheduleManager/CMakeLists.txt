link_directories(PUBLIC ../)

add_library(ScheduleManager STATIC "ScheduleManager.cpp")
  
target_include_directories(WorkerM PUBLIC ${Boost_INCLUDE_DIRS})
target_include_directories(WorkerM PUBLIC ${GlobalModules_ROOT_DIR})

find_package(Boost COMPONENTS thread chrono atomic locale json REQUIRED)

target_link_libraries(ScheduleManager PUBLIC Boost::json)
target_link_libraries(ScheduleManager PUBLIC Boost::locale)
target_link_libraries(ScheduleManager PUBLIC Logger)
target_link_libraries(ScheduleManager PUBLIC JSONFormatter)
 