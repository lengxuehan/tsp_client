cmake_minimum_required(VERSION 3.16)
project(client LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# find needed packages
find_package(Threads REQUIRED)

set_source_files_properties(client.c PROPERTIES LANGUAGE CXX )

include_directories(../../include)
include_directories(../../src)
link_directories(../../lib)

# client
add_executable(${PROJECT_NAME} client.c)
target_link_libraries(${PROJECT_NAME} PRIVATE
        tsp_client
        ssl
        crypto
)
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)
