link_directories(PRIVATE ../)
add_subdirectory("Modules")

add_executable(appMarusiaWorker "main.cpp")

add_library(MarusiaWorker STATIC "MarusiaWorker.cpp")
 
target_include_directories(MarusiaWorker PUBLIC ${Boost_INCLUDE_DIRS})
target_include_directories(MarusiaWorker PUBLIC ${GlobalModules_ROOT_DIR})

find_package(Boost COMPONENTS json thread chrono atomic locale REQUIRED)

target_link_libraries(MarusiaWorker PUBLIC Boost::thread)
target_link_libraries(MarusiaWorker PUBLIC Boost::json)
target_link_libraries(MarusiaWorker PUBLIC Boost::locale)
target_link_libraries(MarusiaWorker PUBLIC WorkerM)
target_link_libraries(MarusiaWorker PUBLIC Configer)
target_link_libraries(MarusiaWorker PUBLIC Logger)
 
target_link_libraries(appMarusiaWorker PUBLIC MarusiaWorker)
