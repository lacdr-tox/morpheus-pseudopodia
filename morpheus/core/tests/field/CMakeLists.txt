
add_executable(FieldTests field_initialization_test.cpp field_diffusion.cpp)
InjectModels(FieldTests)

target_link_libraries(FieldTests PRIVATE ModelTesting gtest gtest_main) # MorpheusCore

# get_property(link_libs TARGET FieldTests PROPERTY LINK_LIBRARIES)
# MESSAGE( STATUS FieldTests Libs: ${link_libs})
# get_property(sources TARGET FieldTests PROPERTY SOURCES)
# MESSAGE( STATUS FieldTests Sources: ${sources})

add_test(NAME FieldTests COMMAND FieldTests)
