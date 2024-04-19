link_directories(PUBLIC ../)
add_library(Worker STATIC "Worker.cpp")


target_include_directories(Worker PUBLIC ${Boost_INCLUDE_DIRS})
target_include_directories(Worker PUBLIC ${OPENSSL_INCLUDE_DIR})
target_include_directories(Worker PUBLIC ${ZLIB_INCLUDE_DIRS}) 
target_include_directories(Worker PUBLIC ${GlobalModules_ROOT_DIR})

find_package(Boost COMPONENTS json thread chrono atomic locale REQUIRED)

target_link_libraries(Worker PUBLIC Boost::thread) 
target_link_libraries(Worker PUBLIC Boost::json)
target_link_libraries(Worker PUBLIC Logger)
target_link_libraries(Worker PUBLIC JSONFormatter)