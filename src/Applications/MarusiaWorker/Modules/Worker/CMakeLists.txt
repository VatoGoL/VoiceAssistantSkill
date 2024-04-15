link_directories(PRIVATE ../)

add_library(Worker STATIC "Worker.cpp")

target_include_directories(Worker PUBLIC ${Boost_INCLUDE_DIRS})
target_include_directories(Worker PUBLIC ${GlobalModules_ROOT_DIR})

find_package(Boost COMPONENTS thread chrono atomic locale json REQUIRED)

target_link_libraries(Worker PUBLIC Boost::thread)
target_link_libraries(Worker PUBLIC Boost::json)
target_link_libraries(Worker PUBLIC Boost::locale)
