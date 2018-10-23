include(CMake_GTest.txt)

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

INCLUDE_DIRECTORIES( ${PROJECT_SOURCE_DIR}/morpheus)
INCLUDE_DIRECTORIES( ${PROJECT_SOURCE_DIR})
INCLUDE_DIRECTORIES( ${PROJECT_BINARY_DIR}/morpheus/core)

# Add test cpp file
add_executable(runMopheusTests
	core/test_vec_h.cpp
	core/test_serialization.cpp 
	evaluator/test_muparser.cpp 
	evaluator/test_evaluator.cpp
	evaluator/stub_simulator.cpp
	$<TARGET_OBJECTS:muParser>
	$<TARGET_OBJECTS:morpheusCore>
)
# Link test executable against gtest & gtest_main
target_link_libraries(runMopheusTests gtest gtest_main xmlParser z tiff)
# Register test to CTest infrastructure
add_test( NAME MorpheusTests COMMAND runMopheusTests )

