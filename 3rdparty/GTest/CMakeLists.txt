# Download and unpack googletest at configure time
SET(GTEST_BASE_PATH ${CMAKE_CURRENT_BINARY_DIR})
file(MAKE_DIRECTORY ${GTEST_BASE_PATH}/download)
configure_file(CMakeLists_GTest.txt.in ${GTEST_BASE_PATH}/download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" -D BUILD_MOCK=OFF .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${GTEST_BASE_PATH}/download )
  
if(result)
  message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()

execute_process(COMMAND ${CMAKE_COMMAND} --build .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${GTEST_BASE_PATH}/download )
  
if(result)
  message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()

# Prevent overriding the parent project's compiler/linker
# settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.
add_subdirectory(${GTEST_BASE_PATH}/src
                 ${GTEST_BASE_PATH}/build
                 EXCLUDE_FROM_ALL)

# The gtest/gtest_main targets carry header search path
# dependencies automatically when using CMake 2.8.11 or
# later. Otherwise we have to add them here ourselves.
if (CMAKE_VERSION VERSION_LESS 2.8.11)
  include_directories("${gtest_SOURCE_DIR}/include")
endif()
