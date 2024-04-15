link_directories(PRIVATE ../)
add_library(SSLSertificate STATIC SSLSertificate.cpp)

target_include_directories(SSLSertificate PUBLIC ${Boost_INCLUDE_DIRS})
target_include_directories(SSLSertificate PUBLIC ${OPENSSL_INCLUDE_DIR})

find_package(OpenSSL COMPONENTS SSL REQUIRED) 

target_link_libraries(SSLSertificate PUBLIC OpenSSL::SSL) 
