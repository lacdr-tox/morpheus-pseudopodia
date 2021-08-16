##################################
# CORE Evaluation and Solver Tests
##################################

add_executable(runExpressionTests
	test_muparser.cpp 
	test_evaluator.cpp
	test_system.cpp
)

# Link test executable against gtest & gtest_main
target_link_libraries_patched(runExpressionTests PRIVATE ModelTesting gtest gtest_main)
get_property(core_if_libs TARGET MorpheusCore PROPERTY INTERFACE_LINK_LIBRARIES)

# Register test to CTest infrastructure
add_test( NAME ExpressionTests COMMAND runExpressionTests )
