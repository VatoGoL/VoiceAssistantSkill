link_directories(PUBLIC ../)

add_library(WorkerM STATIC "WorkerM.cpp")
  
target_include_directories(WorkerM PUBLIC ${Boost_INCLUDE_DIRS})
target_include_directories(WorkerM PUBLIC ${GlobalModules_ROOT_DIR})

find_package(Boost COMPONENTS thread chrono atomic locale json REQUIRED)

target_link_libraries(WorkerM PUBLIC Boost::thread)
target_link_libraries(WorkerM PUBLIC Boost::json)
target_link_libraries(WorkerM PUBLIC Boost::locale)
target_link_libraries(WorkerM PUBLIC Configer)
target_link_libraries(WorkerM PUBLIC Logger)
target_link_libraries(WorkerM PUBLIC JSONFormatter)
