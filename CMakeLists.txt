cmake_minimum_required(VERSION 3.10)
project(YamaBot  VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CURL REQUIRED)

include_directories(
    ${CURL_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/Include
)

add_executable(YamaBot
    Src/Main.CPP
    Src/MessageProcessingCenter.CPP
    Src/InstructionSet.CPP
)

target_link_libraries(YamaBot
    ${CURL_LIBRARIES}
)