
add_executable(MotilityTests directed_motion_test.cpp)
InjectModels(MotilityTests)
target_link_libraries_fixed(MotilityTests PRIVATE ModelTesting gtest gtest_main)

add_test(NAME MotilityTests COMMAND MotilityTests)
