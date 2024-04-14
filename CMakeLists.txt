cmake_minimum_required(VERSION 3.22)

set(BOOST_ROOT "/media/vato/6c438f7c-3d6e-47b1-8c02-ca9aa1dc9ba2/vato/Vato/SmartLift/boost_1_83_0")
set(BOOST_INCLUDEDIR ${BOOST_ROOT})
set(BOOST_LIBRARYDIR ${BOOST_ROOT}/bin/bin.v2/libs)

set(OPENSSL_ROOT_DIR "/media/vato/6c438f7c-3d6e-47b1-8c02-ca9aa1dc9ba2/vato/Vato/SmartLift/openssl/static")
set(OPENSSL_INCLUDE_DIR ${OPENSSL_ROOT_DIR}/include)

set(OPENSSL_NO_SYSTEM_PATHS		 TRUE)
set(OPENSSL_USE_STATIC_LIBS		 TRUE)
set(OPENSSL_MSVC_STATIC_RT		 ON)
set(ZLIB_INCLUDE_DIRS "/media/vato/6c438f7c-3d6e-47b1-8c02-ca9aa1dc9ba2/vato/Vato/tools/zlib/zlib-1.3")
set(ZLIB_USE_STATIC_LIBS ON)

set(Boost_NO_SYSTEM_PATHS		 ON)
set(Boost_USE_STATIC_LIBS        ON)  # only find static libs
set(Boost_USE_DEBUG_LIBS         ON)  # ignore debug libs and
set(Boost_USE_RELEASE_LIBS      ON)  # only find release libs
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)
set(Boost_USE_DYNAMIC_LIBS        OFF)

find_package(Boost REQUIRED) 
find_package(OpenSSL REQUIRED)
 
project(VoiceAssistantSkill)
add_subdirectory("src")