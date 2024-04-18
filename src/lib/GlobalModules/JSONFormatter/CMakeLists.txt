link_directories(PUBLIC ../)

add_library(JSONFormatter STATIC "JSONFormatter.cpp")

target_include_directories(JSONFormatter PUBLIC ${Boost_INCLUDE_DIRS})

find_package(Boost COMPONENTS json REQUIRED)

target_link_libraries(JSONFormatter PUBLIC Boost::json)
