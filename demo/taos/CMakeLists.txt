cmake_minimum_required(VERSION 3.16)
project(taos LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# find needed packages
find_package(Threads REQUIRED)

set_source_files_properties(producer.cpp PROPERTIES LANGUAGE CXX )

include_directories(../include)
link_directories(../lib)

# producer
add_executable(producer producer.cpp)
target_link_libraries(producer PRIVATE
        tsp_client
        ssl
        crypto
        taos
)
set_target_properties(producer PROPERTIES LINKER_LANGUAGE CXX)

# consumer
add_executable(consumer consumer.cpp)
target_link_libraries(consumer PRIVATE
        tsp_client
        ssl
        crypto
        taos
        )
set_target_properties(consumer PROPERTIES LINKER_LANGUAGE CXX)

