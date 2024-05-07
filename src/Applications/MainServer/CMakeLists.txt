link_directories(PUBLIC ../)
add_subdirectory("Modules")

add_executable(appMainServer "main.cpp")

add_library(MainServer STATIC "MainServer.cpp")

target_include_directories(MainServer PUBLIC ${Boost_INCLUDE_DIRS})
target_include_directories(MainServer PUBLIC ${OPENSSL_INCLUDE_DIR})
target_include_directories(MainServer PUBLIC ${ZLIB_INCLUDE_DIRS})
target_include_directories(MainServer PUBLIC ${GlobalModules_ROOT_DIR})
#target_link_directories(MainServer PUBLIC  ${GlobalModules_ROOT_DIR})

find_package(Boost COMPONENTS json thread chrono atomic locale REQUIRED)
find_package(OpenSSL COMPONENTS Crypto REQUIRED)
find_package(OpenSSL COMPONENTS SSL REQUIRED )
find_package(ZLIB COMPONENTS ZLIB REQUIRED)

target_link_libraries(MainServer PUBLIC Boost::thread)
target_link_libraries(MainServer PUBLIC Boost::json)
target_link_libraries(MainServer PUBLIC Boost::locale)
target_link_libraries(MainServer PUBLIC OpenSSL::SSL)
target_link_libraries(MainServer PUBLIC OpenSSL::Crypto)

target_link_libraries(MainServer PUBLIC HTTPSServer)
target_link_libraries(MainServer PUBLIC SSLSertificate)
target_link_libraries(MainServer PUBLIC Worker)
target_link_libraries(MainServer PUBLIC Logger)
target_link_libraries(MainServer PUBLIC Configer)
target_link_libraries(MainServer PUBLIC ClientDB)
target_link_libraries(appMainServer PUBLIC MainServer)
