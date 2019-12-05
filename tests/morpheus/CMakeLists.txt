FIND_PACKAGE(OpenMP)

option(MORPHEUS_OPENMP "Switch off the usage of OPENMP" ON)

IF (OPENMP_FOUND AND MORPHEUS_OPENMP)
	MESSAGE(STATUS "Enabling OpenMP support for ${CMAKE_CXX_COMPILER_ID} using  '${OpenMP_CXX_FLAGS}'")
	SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
	SET(HAVE_OPENMP ON)
ELSEIF(NOT HAVE_OPENMP)
	MESSAGE(STATUS "Unable to detect OpenMP compiler support. Disabling OpenMP.")
	SET(HAVE_OPENMP OFF)
ELSE()
	MESSAGE(STATUS "OpenMP support was switched off manually. Disabling OpenMP. ")
	SET(HAVE_OPENMP OFF)
ENDIF()

# Add test cpp file
add_executable(runCoreTests
	core/test_vec_h.cpp
	core/test_serialization.cpp 
	evaluator/test_muparser.cpp 
	evaluator/test_evaluator.cpp
)

# Link test executable against gtest & gtest_main
target_link_libraries(runCoreTests MorpheusCore gtest gtest_main)
#get_target_property(gtest_inc gtest INCLUDE_DIRECTORIES)
#message(STATUS gtest INCLUDES "${gtest_inc}")
#get_target_property(gtest_inc gtest_main INCLUDE_DIRECTORIES)
#message(STATUS gtest_main INCLUDES "${gtest_inc}")

# Register test to CTest infrastructure
add_test( NAME CoreTests COMMAND runCoreTests )

# Add test cpp file
add_executable(runExpressionTests
	evaluator/test_muparser.cpp 
	evaluator/test_evaluator.cpp
)

# Link test executable against gtest & gtest_main
target_link_libraries(runExpressionTests MorpheusCore gtest gtest_main)

# Register test to CTest infrastructure
add_test( NAME MorpheusTests COMMAND runMopheusTests )

