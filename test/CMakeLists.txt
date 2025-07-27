cmake_minimum_required(VERSION 3.16)

# Enable testing
enable_testing()

# Find testing framework (we'll use a simple built-in approach for now)
# In a real project, you might use GoogleTest, Catch2, or similar

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Test source files
set(TEST_SOURCES
    test_main.cpp
    test_common_types.cpp
    test_audio_config.cpp
    test_ai_scorer.cpp
    test_patch_generator.cpp
    test_json_parser.cpp
    test_interactive_menu.cpp
)

# Create test executable
add_executable(run_tests ${TEST_SOURCES})

# Link against main project libraries (would need to create libraries first)
target_link_libraries(run_tests PRIVATE nlohmann_json::nlohmann_json)

# Compiler flags for tests
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(run_tests PRIVATE -Wall -Wextra -pedantic)
endif()

# Add test to CTest
add_test(NAME AllTests COMMAND run_tests)

# Individual test executables for specific components
set(TEST_COMPONENTS
    common_types
    audio_config
    ai_scorer
    patch_generator
    json_parser
    interactive_menu
)

foreach(COMPONENT ${TEST_COMPONENTS})
    add_executable(test_${COMPONENT} test_main.cpp test_${COMPONENT}.cpp)
    target_link_libraries(test_${COMPONENT} PRIVATE nlohmann_json::nlohmann_json)
    
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(test_${COMPONENT} PRIVATE -Wall -Wextra -pedantic)
    endif()
    
    add_test(NAME Test_${COMPONENT} COMMAND test_${COMPONENT})
endforeach()

# Copy test data files to build directory
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/test_data/test_guitar.json
               ${CMAKE_CURRENT_BINARY_DIR}/test_data/test_guitar.json COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/test_data/test_group.json
               ${CMAKE_CURRENT_BINARY_DIR}/test_data/test_group.json COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/test_data/test_skd.json
               ${CMAKE_CURRENT_BINARY_DIR}/test_data/test_skd.json COPYONLY)