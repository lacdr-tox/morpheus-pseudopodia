

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
)

IF (Boost_FOUND) 
  LIST(APPEND morpheus_core_src delay.cpp)
ENDIF()

SET(morpheus_src
	parse_arg.cpp
	rss_stat.cpp
	simulation.cpp
)

target_sources_relpaths(MorpheusSIM PRIVATE morpheus_src)
target_sources_relpaths(MorpheusCore PRIVATE morpheus_core_src)
target_include_directories(MorpheusSIM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(MorpheusCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

SET(APP_DOC_SOURCES
	interfaces.h
	docu.h
	equation.h
	expression_evaluator.h
	field.h
	diffusion.h
	vector_equation.h 
	function.h 
	membrane_property.h
	property.h
	delay.h
	system.h
	cpm_sampler.h
)

list_prepend( APP_DOC_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/" APP_DOC_SOURCES )
SET_PROPERTY( TARGET AppDocLib PROPERTY SOURCES "${APP_DOC_SOURCES}" ) 
# target_sources(AppDocLib PRIVATE ${APP_DOC_SOURCES})


# Also link all the plugins from the plugin folder
foreach(source ${plugin_src})
	list(APPEND morpheus_src ../plugins/${source})
endforeach(source)

# MESSAGE( "TIFF INCLUDE = " ${TIFF_INCLUDE_DIR})
CONFIGURE_FILE(config.h.cmake config.h)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR} ${TIFF_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR})
 
option(MORPHEUS_CORE_CATCH "Catch exceptions in the simulator core. Required for GUI integration. May be switched off for debuging purposese. " ON)
mark_as_advanced(MORPHEUS_CORE_CATCH)

if (NOT(MORPHEUS_CORE_CATCH))
# 	ADD_DEFINITIONS("-D NO_CORE_CATCH")
	SET_PROPERTY(
		SOURCE simulation.cpp
		PROPERTY COMPILE_DEFINITIONS "NO_CORE_CATCH"
		)
ENDIF()


IF  ("${MORPHEUS_OS}" STREQUAL "WIN32")
	TARGET_LINK_LIBRARIES(MorpheusSIM pthread psapi)

# 	If (MORPHEUS_STATIC_BUILD)
# 		set(STD_LIB_SUFFIX ${CMAKE_FIND_LIBRARY_SUFFIXES})
# 		set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
# 	ENDIF()
# 	
# 	if ( HAVE_OPENMP AND MORPHEUS_STATIC_BUILD)
# 		SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lpthread")
# # 		FIND_LIBRARY(PTHREADS_LIBRARY NAMES pthread)
# # 		LINK_LIBRARIES( ${PTHREADS_LIBRARY} )
# 	ENDIF()
# 	SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lpsapi")
# # 	FIND_LIBRARY(PSAPI_LIBRARY NAMES psapi)
# # 	LINK_LIBRARIES( ${PSAPI_LIBRARY} )
# 	# find_library(PTHREADS_LIBRARY NAMES libpthread)
# 	If (MORPHEUS_STATIC_BUILD)
# 		set(CMAKE_FIND_LIBRARY_SUFFIXES ${STD_LIB_SUFFIX})
# 	ENDIF()
ENDIF()



# Installation of the program
IF  ( "${MORPHEUS_OS}" STREQUAL "WIN32")
	INSTALL(TARGETS ${SIMULATOR_EXEC_NAME} RUNTIME DESTINATION . COMPONENT morpheus )
	find_library(LIB_STDCXX "libstdc++-6" )
	find_library(LIB_GCC_S_DW2 "libgcc_s_dw2-1")
	find_library(LIB_OPENMP "libgomp-1")
	find_library(LIB_PTHREAD "pthreadGC-3")
	INSTALL( FILES ${LIB_STDCXX} ${LIB_GCC_S_DW2} ${LIB_OPENMP} ${LIB_PTHREAD} DESTINATION . )
	
	IF (GRAPHVIZ_FOUND)
		SET(GRAPHVIZ_RUNTIME
			${GRAPHVIZ_ROOT}/bin/config6
			${GRAPHVIZ_ROOT}/bin/libexpat.dll
			${GRAPHVIZ_ROOT}/bin/ltdl.dll
			${GRAPHVIZ_ROOT}/bin/zlib1.dll
			${GRAPHVIZ_ROOT}/lib/release/dll/ann.dll
			${GRAPHVIZ_ROOT}/lib/release/dll/cdt.dll
			${GRAPHVIZ_ROOT}/lib/release/dll/cgraph.dll
			${GRAPHVIZ_ROOT}/lib/release/dll/gvc.dll
			${GRAPHVIZ_ROOT}/lib/release/dll/gvplugin_core.dll
			${GRAPHVIZ_ROOT}/lib/release/dll/gvplugin_dot_layout.dll
			${GRAPHVIZ_ROOT}/lib/release/dll/gvplugin_gd.dll
			${GRAPHVIZ_ROOT}/lib/release/dll/gvplugin_gdiplus.dll
			${GRAPHVIZ_ROOT}/lib/release/dll/gvplugin_neato_layout.dll
			${GRAPHVIZ_ROOT}/lib/release/dll/gvplugin_pango.dll
			${GRAPHVIZ_ROOT}/lib/release/dll/Pathplan.dll
			${GRAPHVIZ_ROOT}/lib/release/dll/vmalloc.dll
		)
		INSTALL(FILES ${GRAPHVIZ_RUNTIME} DESTINATION . COMPONENT GraphViz)
	ENDIF()
	
	SET(GNUPLOT_ROOT "" CACHE PATH "Set root directory to search for Gnuplot" )
	MARK_AS_ADVANCED(GNUPLOT_ROOT)
	
	message(STATUS "Gnuplot Path is ${GNUPLOT_ROOT}")
	SET(GNUPLOT_LIBRARIES 
		${GNUPLOT_ROOT}/freetype6.dll
		${GNUPLOT_ROOT}/intl.dll
		${GNUPLOT_ROOT}/libcairo-2.dll
		${GNUPLOT_ROOT}/libexpat-1.dll
		${GNUPLOT_ROOT}/libfontconfig-1.dll
		${GNUPLOT_ROOT}/libgd-2-733361a31aab.dll
		${GNUPLOT_ROOT}/libglib-2.0-0.dll
		${GNUPLOT_ROOT}/libgmodule-2.0-0.dll
		${GNUPLOT_ROOT}/libgobject-2.0-0.dll
		${GNUPLOT_ROOT}/libgthread-2.0-0.dll
		${GNUPLOT_ROOT}/libiconv-2.dll
		${GNUPLOT_ROOT}/libjpeg-8.dll
		${GNUPLOT_ROOT}/libpango-1.0-0.dll
		${GNUPLOT_ROOT}/libpangocairo-1.0-0.dll
		${GNUPLOT_ROOT}/libpangoft2-1.0-0.dll
		${GNUPLOT_ROOT}/libpangowin32-1.0-0.dll
		${GNUPLOT_ROOT}/libpng14-14.dll
		${GNUPLOT_ROOT}/lua51.dll
		${GNUPLOT_ROOT}/wxbase28_gcc_custom.dll
		${GNUPLOT_ROOT}/wxmsw28_core_gcc_custom.dll
		${GNUPLOT_ROOT}/zlib1.dll
	)
	INSTALL(FILES ${GNUPLOT_LIBRARIES} DESTINATION . COMPONENT GnuPlot)
	INSTALL(FILES ${GNUPLOT_ROOT}/gnuplot.exe DESTINATION . COMPONENT GnuPlot)
		
ELSE()
	INSTALL(TARGETS ${SIMULATOR_EXEC_NAME} RUNTIME DESTINATION bin COMPONENT morpheus )
ENDIF()
