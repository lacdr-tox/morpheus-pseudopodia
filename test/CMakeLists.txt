add_executable(
	runAllTests
        main.cpp
        morpheus/core/test_vec.cpp
)
include_directories(${CMAKE_BINARY_DIR}/morpheus/core) # for config.h
target_link_libraries(runAllTests gtest_main)
add_test(NAME all_tests COMMAND runAllTests)
