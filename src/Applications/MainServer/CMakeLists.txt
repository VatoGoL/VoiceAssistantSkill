link_directories(PRIVATE ../../../)
add_subdirectory("Modules")

add_executable(appMainServer "main.cpp")

add_library(MainServer STATIC "MainServer.cpp")

target_include_directories(MainServer PUBLIC ${Boost_INCLUDE_DIRS})
target_include_directories(MainServer PUBLIC ${OPENSSL_INCLUDE_DIR})
target_include_directories(MainServer PUBLIC ${ZLIB_INCLUDE_DIRS})

#message("Boost_INCLUDE_DIRS " ${Boost_INCLUDE_DIRS})
#message("OPENSSL_INCLUDE_DIR " ${OPENSSL_INCLUDE_DIR})
#message("ZLIB_INCLUDE_DIRS " ${ZLIB_INCLUDE_DIRS})

find_package(Boost COMPONENTS thread chrono atomic locale REQUIRED)
find_package(Boost COMPONENTS json REQUIRED)
find_package(OpenSSL COMPONENTS Crypto REQUIRED)
find_package(OpenSSL COMPONENTS SSL REQUIRED )
find_package(ZLIB COMPONENTS ZLIB REQUIRED)

target_link_libraries(MainServer PUBLIC Boost::thread)
target_link_libraries(MainServer PUBLIC Boost::json)
target_link_libraries(MainServer PUBLIC Boost::locale)
target_link_libraries(MainServer PUBLIC OpenSSL::SSL)
target_link_libraries(MainServer PUBLIC OpenSSL::Crypto)

target_link_libraries(MainServer PRIVATE HTTPSServer)
target_link_libraries(MainServer PRIVATE SSLSertificate)
target_link_libraries(appMainServer PRIVATE MainServer)
