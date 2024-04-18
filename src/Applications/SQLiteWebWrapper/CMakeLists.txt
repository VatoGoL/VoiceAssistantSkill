link_directories(PUBLIC ../)
add_subdirectory("Modules") 
  
add_executable(appSQLiteWebWrapper "main.cpp")

add_library(SQLiteWebWrapper  STATIC "SQLiteWebWrapper.cpp")

target_include_directories(SQLiteWebWrapper  PUBLIC ${Boost_INCLUDE_DIRS})
target_include_directories(SQLiteWebWrapper  PUBLIC ${GlobalModules_ROOT_DIR})

find_package(Boost COMPONENTS json thread chrono atomic locale REQUIRED)

target_link_libraries(SQLiteWebWrapper  PUBLIC Boost::thread)
target_link_libraries(SQLiteWebWrapper  PUBLIC Boost::json)
target_link_libraries(SQLiteWebWrapper  PUBLIC Boost::locale)
target_link_libraries(SQLiteWebWrapper  PUBLIC Worker)

target_link_libraries(appSQLiteWebWrapper PRIVATE SQLiteWebWrapper )
