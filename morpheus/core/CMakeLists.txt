SET(morpheus_core_src
	cell.cpp
	celltype.cpp
	cell_update.cpp
	cpm.cpp
	cpm_layer.cpp
	cpm_sampler.cpp
	cpm_shape.cpp
	cpm_shape_tracker.cpp
	data_mapper.cpp
	domain.cpp
	diffusion.cpp
	diff_eqn.cpp
	edge_tracker.cpp
	equation.cpp
	evaluator_cache.cpp
	expression_evaluator.cpp
	field.cpp
	focusrange.cpp
	function.cpp
	interaction_energy.cpp
	interfaces.cpp
	lattice.cpp
	lattice_plugin.cpp
	membrane_property.cpp
	membranemapper.cpp
	plugin_parameter.cpp
	property.cpp
	random_functions.cpp
	scales.cpp
	scope.cpp
	string_functions.cpp
	symbol.cpp
	symbolfocus.cpp
	system.cpp
	time_scheduler.cpp
	vector_equation.cpp
	xml_functions.cpp
# 	parse_arg.cpp
	rss_stat.cpp
	simulation.cpp
)

IF (Boost_FOUND) 
  LIST(APPEND morpheus_core_src delay.cpp)
ENDIF()

SET(morpheus_src
	main.cpp
)

target_sources_relpaths(MorpheusSIM PRIVATE morpheus_src)
target_sources_relpaths(MorpheusCore PRIVATE morpheus_core_src)

target_include_directories(MorpheusCore PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

CONFIGURE_FILE(config.h.cmake config.h)
 
option(MORPHEUS_CORE_CATCH "Catch exceptions in the simulator core. Required for GUI integration. May be switched off for debuging purposese. " ON)
mark_as_advanced(MORPHEUS_CORE_CATCH)

if (NOT(MORPHEUS_CORE_CATCH))
	SET_PROPERTY(
		SOURCE simulation.cpp
		PROPERTY COMPILE_DEFINITIONS "NO_CORE_CATCH"
		)
ENDIF()

IF(MORPHEUS_TESTS)
	add_subdirectory(tests)
ENDIF()

