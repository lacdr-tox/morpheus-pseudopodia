IF(MORPHEUS_STATIC_BUILD)
	set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
ENDIF()

FIND_PACKAGE(ZLIB REQUIRED)
FIND_PACKAGE(TIFF REQUIRED)

SET(GRAPHVIZ_ROOT "" CACHE PATH "Set root directory to search for GraphViz" )
MARK_AS_ADVANCED(GRAPHVIZ_ROOT)
FIND_PACKAGE(Graphviz)

FIND_PACKAGE(Boost REQUIRED)
IF (Boost_FOUND) 
	SET(HAVE_BOOST 1)
ELSE()
	MESSAGE(STATUS "Unable to detect Boost library. Turning off Delay support.")
ENDIF()

#IF( NOT GNUPLOT_FOUND )
#	MESSAGE("Cannot find Gnuplot. To use the Gnuplotter visualization plugin, install a recent version (4.0+) of gnuplot.")
#ENDIF ()

INCLUDE(CheckIncludeFiles)
INCLUDE(CheckIncludeFileCXX)

CHECK_INCLUDE_FILES("sys/types.h;sys/stat.h;sys/unistd.h" HAVE_GNU_SYSLIB_H)

# Configuration header file for the platform
INCLUDE (CheckTypeSize) 
CHECK_TYPE_SIZE(uint UINT)
# sets HAVE_UINT

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

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR} ${Boost_INCLUDE_DIR})


IF (HAVE_OBJECT_LINKAGE)
	ADD_LIBRARY(MorpheusCore OBJECT ""  )
	ADD_EXECUTABLE(MorpheusSIM "")
	target_link_libraries(MorpheusSIM $<TARGET_OBJECTS:muParser> $<TARGET_OBJECTS:MorpheusCore>)
ELSE()
	ADD_LIBRARY(MorpheusCore ""  )
	ADD_EXECUTABLE(MorpheusSIM "" )
	target_link_libraries(MorpheusSIM muParser MorpheusCore)
ENDIF()
set_target_properties(MorpheusSIM PROPERTIES  OUTPUT_NAME ${MORPHEUS_EXEC_NAME} )
IF (MORPHEUS_STATIC_BUILD)
	SET_TARGET_PROPERTIES(MorpheusSIM PROPERTIES LINK_SEARCH_START_STATIC 1)
	SET_TARGET_PROPERTIES(MorpheusSIM PROPERTIES LINK_SEARCH_END_STATIC 1)
	SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ ")
# 	set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
ENDIF()

TARGET_LINK_LIBRARIES(MorpheusSIM ${ZLIB_LIBRARY} ${TIFF_LIBRARY} xmlParser gnuplot_interface tiny-process-library)

IF (EXISTS "${GRAPHVIZ_GRAPH_LIBRARY}")
	TARGET_LINK_LIBRARIES(MorpheusSIM ${GRAPHVIZ_GVC_LIBRARY} ${GRAPHVIZ_GRAPH_LIBRARY} )
ELSE()
	TARGET_LINK_LIBRARIES(MorpheusSIM ${GRAPHVIZ_GVC_LIBRARY} ${GRAPHVIZ_CGRAPH_LIBRARY} )
ENDIF()

add_subdirectory(plugins)
add_subdirectory(core)


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
	INSTALL(TARGETS MorpheusSIM RUNTIME DESTINATION bin COMPONENT morpheus )
ENDIF()



#####################################
##  Docu and Language definition  ##
#####################################

SET(XSD_SOURCES 
	${CMAKE_CURRENT_SOURCE_DIR}/core/base_types.xsd
	${CMAKE_CURRENT_SOURCE_DIR}/core/simulation.xsd
	${CMAKE_CURRENT_SOURCE_DIR}/core/lattice.xsd
)

SET(APP_DOC_HEADERS 
	${CMAKE_CURRENT_SOURCE_DIR}/core/docu.h
	${CMAKE_CURRENT_SOURCE_DIR}/core/expression_evaluator.h
)

get_target_property(core_sources MorpheusCore SOURCES)
get_target_property(sources MorpheusSIM SOURCES)

# string(GENEX_STRIP ${sources} sources)
SET(sources ${sources} ${core_sources})

foreach(source ${sources})
	STRING(REGEX REPLACE "\\.cpp" ".h" header ${source})
	STRING(REGEX REPLACE "\\.cpp" ".xsd" xsd ${source})
	#  check the header for the PLUGIN macro
	file(STRINGS "${header}" lines REGEX "DECLARE_PLUGIN")
	if (lines)
		list(APPEND APP_DOC_HEADERS ${header})
		if ( EXISTS ${xsd} )
			list(APPEND XSD_SOURCES ${xsd})
		else()
			if ( header MATCHES "interfaces.h")
				# just the macro definition
			else()
				message(WARNING "Missing plugin language description ${xsd}" )
			endif()
		endif()
	ENDIF()
endforeach(source)


SET(XML_MERGE_TRANS_SCRIPT  "${CMAKE_CURRENT_SOURCE_DIR}/merge.xslt")

FIND_PROGRAM(XSLTPROC "xsltproc" NAMES "xsltproc xsltproc.exe" DOC "XSLTProc executable")
FIND_PROGRAM(XMLLINT "xmllint" NAMES "xmllint xsltproc.exe" DOC "XMLLint executable")

IF( NOT XSLTPROC )
	message(FATAL_ERROR "Unable to locate 'xsltproc' executable")
ENDIF()


SET(XSD_NAME "${PROJECT_NAME}.xsd")

UNSET(MERGE_INTERMEDIATE)
FOREACH( xsd_file ${XSD_SOURCES})
	IF ( DEFINED MERGE_INTERMEDIATE )
		get_filename_component(xname ${xsd_file} NAME )
		SET(MERGE_OUTPUT "${xname}_merged")
		STRING(REPLACE " " "%20" SCRIPT_SAVE_INTERMEDIATE ${MERGE_INTERMEDIATE})
		SET(SCRIPT_SAVE_INTERMEDIATE \\\"${SCRIPT_SAVE_INTERMEDIATE}\\\")
		ADD_CUSTOM_COMMAND(
			OUTPUT ${MERGE_OUTPUT}
			COMMAND ${XSLTPROC} -o ${MERGE_OUTPUT} --param with ${SCRIPT_SAVE_INTERMEDIATE} ${XML_MERGE_TRANS_SCRIPT} "${xsd_file}"
			DEPENDS ${xsd_file} ${MERGE_INTERMEDIATE} ${XML_MERGE_TRANS_SCRIPT}
			COMMENT "Merging in ${xsd_file} into ${XSD_NAME}"
		)
		SET(MERGE_INTERMEDIATE "${CMAKE_CURRENT_BINARY_DIR}/${MERGE_OUTPUT}")
	ELSE()
		SET(MERGE_INTERMEDIATE ${xsd_file})
	ENDIF()
ENDFOREACH()
SET(MERGED_SCHEMA ${MERGE_INTERMEDIATE})


IF ( "${XMLLINT}" STREQUAL "XMLLINT-NOTFOUND" )
	MESSAGE( STATUS "Unable to locate 'xmllint' executable.\nSkipping xsd pretty printing.")
	ADD_CUSTOM_COMMAND(
		OUTPUT ${XSD_NAME}
		COMMAND cmake -E copy ${MERGED_SCHEMA} ${XSD_NAME}
		DEPENDS ${MERGED_SCHEMA} 
	)
ELSE()
	ADD_CUSTOM_COMMAND(
		OUTPUT ${XSD_NAME}
		COMMAND ${XMLLINT} --format --noblanks --output ${XSD_NAME} ${MERGED_SCHEMA}
		DEPENDS ${MERGED_SCHEMA}
		COMMENT "Pretty formatting XML schema ${XSD_NAME}"
	)
ENDIF()

ADD_CUSTOM_TARGET( 
	xmlSchema
	DEPENDS ${XSD_NAME} ${XSD_SOURCES}
)


SET(APP_DOC_HEADERS ${APP_DOC_HEADERS} PARENT_SCOPE)
SET(MORPHEUS_XSD_FILE ${CMAKE_CURRENT_BINARY_DIR}/${XSD_NAME} PARENT_SCOPE)


