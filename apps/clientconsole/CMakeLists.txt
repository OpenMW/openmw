
find_package(Threads REQUIRED)
add_executable(clientconsole client.cpp)
target_link_libraries(clientconsole ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
