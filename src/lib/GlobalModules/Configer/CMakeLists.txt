link_directories(PUBLIC ../)

add_library(Configer STATIC "Configer.cpp")

target_include_directories(Configer PUBLIC ${Boost_INCLUDE_DIRS})
target_include_directories(Configer PUBLIC ${GlobalModules_ROOT_DIR})

find_package(Boost COMPONENTS thread chrono atomic locale json REQUIRED)

target_link_libraries(Configer PUBLIC Boost::thread)
target_link_libraries(Configer PUBLIC Boost::json)
target_link_libraries(Configer PUBLIC Boost::locale)
