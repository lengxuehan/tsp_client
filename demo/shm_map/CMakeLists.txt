cmake_minimum_required(VERSION 3.16)
project(shm_map LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# find needed packages
find_package(Threads REQUIRED)

set_source_files_properties(shm_map_test.cpp PROPERTIES LANGUAGE CXX )

include_directories(../include)
link_directories(../lib)

# client
add_executable(${PROJECT_NAME} shm_map_test.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE
        tsp_client
        ssl
        crypto
)
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

