link_directories(PUBLIC ../)

add_library(ClientDB STATIC "ClientDB.cpp")

target_include_directories(ClientDB PUBLIC ${Boost_INCLUDE_DIRS})
target_include_directories(ClientDB PUBLIC ${GlobalModules_ROOT_DIR})

find_package(Boost COMPONENTS thread chrono atomic locale json REQUIRED)

target_link_libraries(ClientDB PUBLIC Boost::thread)
target_link_libraries(ClientDB PUBLIC Boost::json)
target_link_libraries(ClientDB PUBLIC Boost::locale)
