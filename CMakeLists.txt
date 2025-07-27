cmake_minimum_required(VERSION 3.16)
project(AIInstrumentSynthesis)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(nlohmann_json REQUIRED)

# Set directory structure
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test)

# Include directories
include_directories(${INCLUDE_DIR})

# Source files
set(SOURCES
    ${SRC_DIR}/main.cpp
    ${SRC_DIR}/audio_config.cpp
    ${SRC_DIR}/json_parser.cpp
    ${SRC_DIR}/ai_scorer.cpp
    ${SRC_DIR}/patch_generator.cpp
    ${SRC_DIR}/interactive_menu.cpp
)

# Header files
set(HEADERS
    ${INCLUDE_DIR}/audio_config.h
    ${INCLUDE_DIR}/json_parser.h
    ${INCLUDE_DIR}/ai_scorer.h
    ${INCLUDE_DIR}/patch_generator.h
    ${INCLUDE_DIR}/interactive_menu.h
    ${INCLUDE_DIR}/common_types.h
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)

# Enable testing
enable_testing()
add_subdirectory(${TEST_DIR})

# Compiler flags
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)
endif()