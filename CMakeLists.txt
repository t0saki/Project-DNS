cmake_minimum_required(VERSION 3.10)

project(dns_project C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build)

add_executable(dns_client src/client/client.c src/utils/dns_message.h)
# add_executable(dns_server server.c dns_message.c)