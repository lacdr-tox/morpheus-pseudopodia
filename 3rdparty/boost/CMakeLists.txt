configure_file(ep_boost-CMakeLists.txt.in boost-download/CMakeLists.txt @ONLY)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/boost-download )
if(result)
  message(FATAL_ERROR "CMake step for boost failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/boost-download )
if(result)
  message(FATAL_ERROR "Build step for boost failed: ${result}")
endif()

set(ENV{BOOST_ROOT} ${CMAKE_CURRENT_BINARY_DIR}/boost-build)
