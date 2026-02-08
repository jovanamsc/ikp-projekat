cmake_minimum_required(VERSION 3.16)
project(ikp_ahm LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(ahm
    ahm/ahm.cpp
    heap_manager/ahm_manager.cpp
)

target_include_directories(ahm PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/ahm
    ${CMAKE_CURRENT_SOURCE_DIR}/heap_manager
)

if (WIN32)
    target_link_libraries(ahm PRIVATE kernel32)
endif()

add_executable(test_app
    tests/test_app/test_app.cpp
)

target_link_libraries(test_app PRIVATE ahm)

add_executable(test_server
    tests/test_server/test_server.cpp
)

target_link_libraries(test_server PRIVATE ahm)

add_executable(test_client
    tests/test_client/test_client.cpp
)

add_executable(test_threads
    tests/test_threads/test_threads.cpp
)

target_link_libraries(test_threads PRIVATE ahm)

target_link_libraries(test_client PRIVATE ahm)

if (WIN32)
    target_link_libraries(test_server PRIVATE ws2_32)
    target_link_libraries(test_client PRIVATE ws2_32)
    target_link_libraries(test_threads PRIVATE ws2_32)
endif()
