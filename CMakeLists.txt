cmake_minimum_required(VERSION 3.16)

project(ikp_ahm LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =========================
# Biblioteka: ahm
# =========================
add_library(ahm
    Projekat/ahm/ahm.cpp
    Projekat/heap_manager/ahm_manager.cpp
)

target_include_directories(ahm PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Projekat/ahm
    ${CMAKE_CURRENT_SOURCE_DIR}/Projekat/heap_manager
)

if (WIN32)
    target_link_libraries(ahm PRIVATE kernel32)
endif()

# =========================
# Test executables
# =========================
add_executable(test_app
    Projekat/tests/test_app/test_app.cpp
)
target_link_libraries(test_app PRIVATE ahm)

add_executable(test_server
    Projekat/tests/test_server/test_server.cpp
)
target_link_libraries(test_server PRIVATE ahm)

add_executable(test_client
    Projekat/tests/test_client/test_client.cpp
)
target_link_libraries(test_client PRIVATE ahm)

add_executable(test_threads
    Projekat/tests/test_threads/test_threads.cpp
)
target_link_libraries(test_threads PRIVATE ahm)

# =========================
# Windows-specific libs
# =========================
if (WIN32)
    target_link_libraries(test_server PRIVATE ws2_32)
    target_link_libraries(test_client PRIVATE ws2_32)
    target_link_libraries(test_threads PRIVATE ws2_32)
endif()
