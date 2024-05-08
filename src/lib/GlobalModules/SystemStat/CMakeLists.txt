link_directories(PUBLIC ../)

add_library(SystemStat STATIC "SystemStat.cpp")
  
target_include_directories(SystemStat PUBLIC ${GlobalModules_ROOT_DIR})

target_link_libraries(SystemStat PUBLIC Logger)

