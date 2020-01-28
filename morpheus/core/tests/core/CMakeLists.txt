###################
# CORE Tests
###################

add_executable(runCoreTests
	test_vec_h.cpp
	test_serialization.cpp 
)
target_link_libraries_patched(runCoreTests PRIVATE ModelTesting gtest gtest_main)

# Register test to CTest infrastructure
add_test( NAME CoreTests COMMAND runCoreTests )
