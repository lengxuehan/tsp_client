cmake_minimum_required(VERSION 3.5)
project(tsp_client)

# Cmake options
option(BUILD_SHARED_LIBS "Option to build as shared library" OFF)
option(BUILD_WITH_DLT "Option to use Dlt for logging" OFF)
option(BUILD_DOXYGEN "Option to generate doxygen file" OFF)
option(BUILD_WITH_TEST "Option to build test target" OFF)
option(BUILD_EXAMPLES "Option to build example targets" OFF)

# add compiler preprocessor flag when dlt enabled
if (BUILD_WITH_DLT)
    add_compile_definitions(ENABLE_DLT_LOGGER)
    message("Dlt logging enabled in diag-client library")
endif (BUILD_WITH_DLT)

#set default language
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# find needed packages
find_package(Threads REQUIRED)

set(CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS} -fPIC")

# main sources
file(GLOB_RECURSE BOOST_SUPPORT_SRCS_TCP "${CMAKE_CURRENT_SOURCE_DIR}/src/boost_support/socket/tcp/*.cpp")
file(GLOB_RECURSE BOOST_SUPPORT_SRCS_UDP "${CMAKE_CURRENT_SOURCE_DIR}/src/boost_support/socket/udp/*.cpp")
file(GLOB_RECURSE SRCS_COMMON "${CMAKE_CURRENT_SOURCE_DIR}/src/common/*.cpp")
file(GLOB_RECURSE SRCS_CLIENT "${CMAKE_CURRENT_SOURCE_DIR}/src/client/*.cpp")
file(GLOB_RECURSE SRCS_TINYXML2 "${CMAKE_CURRENT_SOURCE_DIR}/src/tinyxml2/*.cpp")
file(GLOB_RECURSE SRCS_TRANSPORT "${CMAKE_CURRENT_SOURCE_DIR}/src/transport/*.cpp")
file(GLOB_RECURSE SRCS_UTILITY "${CMAKE_CURRENT_SOURCE_DIR}/src/utility/*.cpp")
file(GLOB_RECURSE SRCS_MESSAGES "${CMAKE_CURRENT_SOURCE_DIR}/src/packages/*.cpp")
file(GLOB_RECURSE SRCS_HTTP_CLIENT "${CMAKE_CURRENT_SOURCE_DIR}/src/http_client/*.cpp")
file(GLOB_RECURSE SRCS_SHM_MAP "${CMAKE_CURRENT_SOURCE_DIR}/src/shm_map/*.c")
file(GLOB_RECURSE SRCS_OPENSSL_WRAPPER "${CMAKE_CURRENT_SOURCE_DIR}/src/openssl_wrapper/*.cpp")

include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src)

#link_libraries(libssl.a libcrypto.so)

add_library(${PROJECT_NAME} SHARED
        ${BOOST_SUPPORT_SRCS_TCP}
        ${BOOST_SUPPORT_SRCS_UDP}
        ${SRCS_COMMON}
        ${SRCS_CLIENT}
        ${SRCS_TINYXML2}
        ${SRCS_TRANSPORT}
        ${SRCS_UTILITY}
        ${SRCS_MESSAGES}
        ${SRCS_HTTP_CLIENT}
        ${SRCS_SHM_MAP}
        ${SRCS_OPENSSL_WRAPPER}
        )

# set dynamic libray output path
set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib)

# link all dynamic libraries
target_link_libraries(${PROJECT_NAME} PUBLIC
        Threads::Threads
        ssl
        crypto
        curl
        rt
        )

add_subdirectory(demo/server)
add_subdirectory(demo/http_client)
add_subdirectory(demo/shm_map)
add_subdirectory(demo/signature_test)