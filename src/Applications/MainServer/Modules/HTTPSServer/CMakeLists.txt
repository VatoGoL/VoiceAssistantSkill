link_directories(PRIVATE ../)
add_library(HTTPSServer STATIC HTTPSServer.cpp)

target_include_directories(HTTPSServer PUBLIC ${Boost_INCLUDE_DIRS})
target_include_directories(HTTPSServer PUBLIC ${OPENSSL_INCLUDE_DIR})
target_include_directories(HTTPSServer PUBLIC ${ZLIB_INCLUDE_DIRS})

find_package(OpenSSL COMPONENTS SSL Crypto REQUIRED) 
find_package(Boost COMPONENTS json thread chrono atomic locale REQUIRED)
find_package(ZLIB COMPONENTS ZLIB REQUIRED)

target_link_libraries(HTTPSServer PUBLIC OpenSSL::SSL) 
target_link_libraries(HTTPSServer PUBLIC Boost::thread)
target_link_libraries(HTTPSServer PUBLIC Boost::json)
target_link_libraries(HTTPSServer PUBLIC OpenSSL::SSL)
target_link_libraries(HTTPSServer PUBLIC OpenSSL::Crypto)
target_link_libraries(HTTPSServer PUBLIC ZLIB::ZLIB)