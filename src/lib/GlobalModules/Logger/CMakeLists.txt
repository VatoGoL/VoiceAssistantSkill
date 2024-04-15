link_directories(PRIVATE ../)

add_library(Logger STATIC "Logger.cpp")

target_include_directories(Logger PUBLIC ${Boost_INCLUDE_DIRS})

find_package(Boost COMPONENTS thread chrono atomic locale json REQUIRED)

target_link_libraries(Logger PUBLIC Boost::thread)
target_link_libraries(Logger PUBLIC Boost::json)
target_link_libraries(Logger PUBLIC Boost::locale)
