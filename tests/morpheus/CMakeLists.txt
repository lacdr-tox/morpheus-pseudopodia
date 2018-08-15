include(CMake_GTest.txt)

INCLUDE_DIRECTORIES( ${PROJECT_SOURCE_DIR}/morpheus)
INCLUDE_DIRECTORIES( ${PROJECT_BINARY_DIR}/morpheus/core)

# Add test cpp file
add_executable(runMopheusTests core/test_vec_h.cpp core/test_serialization.cpp)
# Link test executable against gtest & gtest_main
target_link_libraries(runMopheusTests gtest gtest_main)
# Register test to CTest infrastructure
add_test( NAME MorpheusTests COMMAND runMopheusTests )
