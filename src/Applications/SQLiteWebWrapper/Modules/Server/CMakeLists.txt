link_directories(PUBLIC ../)
 
set(SQLiteLib_ROOT_DIR "/media/vato/6c438f7c-3d6e-47b1-8c02-ca9aa1dc9ba2/vato/Vato/Diplom/SQLiteLib")

add_library(SQLiteAPI STATIC ${SQLiteLib_ROOT_DIR}/SQLiteAPI/sqlite3.c)

add_library(DataBaseServer STATIC "DataBaseServer.cpp")

target_include_directories(DataBaseServer PUBLIC ${SQLiteLib_ROOT_DIR})
target_include_directories(DataBaseServer PUBLIC ${Boost_INCLUDE_DIRS})
target_include_directories(DataBaseServer PUBLIC ${GlobalModules_ROOT_DIR})

find_package(Boost COMPONENTS json thread chrono atomic REQUIRED)
target_link_libraries(DataBaseServer PUBLIC Boost::thread)
target_link_libraries(DataBaseServer PUBLIC Boost::json)
target_link_libraries(DataBaseServer PUBLIC Logger)
target_link_libraries(DataBaseServer PUBLIC Configer)
target_link_libraries(DataBaseServer PUBLIC JSONFormatter)
target_link_libraries(DataBaseServer PUBLIC SQLiteAPI)

