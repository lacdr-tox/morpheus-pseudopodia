INCLUDE_DIRECTORIES( ${PROJECT_SOURCE_DIR}/morpheus/core)
INCLUDE_DIRECTORIES( ${PROJECT_BINARY_DIR}/morpheus/core)

# Add test cpp file
add_executable(runVecTests
    test_vec_h.cpp
)
# Link test executable against gtest & gtest_main
target_link_libraries(runVecTests gtest gtest_main)

add_test( NAME VecTests COMMAND runVecTests
)
