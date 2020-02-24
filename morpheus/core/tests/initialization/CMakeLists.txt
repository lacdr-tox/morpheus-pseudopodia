
add_executable(InitTests const_initialization_test.cpp)
InjectModels(InitTests)
target_link_libraries(InitTests PRIVATE ModelTesting gtest gtest_main) # MorpheusCore

add_test(NAME InitTests COMMAND InitTests)
