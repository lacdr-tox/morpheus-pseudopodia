
FIND_PACKAGE(ZLIB REQUIRED)

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
IF (GRAPHVIZ_FOUND)
	include_directories( ${GRAPHVIZ_INCLUDE_DIR} )
ENDIF()

add_subdirectory(plugins)  # provides plugin_src variable as a list of Plugins
add_subdirectory(core)

SET (APP_DOC_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/core/interfaces.h
	${CMAKE_CURRENT_SOURCE_DIR}/core/docu.h
	${CMAKE_CURRENT_SOURCE_DIR}/core/equation.h
	${CMAKE_CURRENT_SOURCE_DIR}/core/expression_evaluator.h
	${CMAKE_CURRENT_SOURCE_DIR}/core/field.h
	${CMAKE_CURRENT_SOURCE_DIR}/core/diffusion.h
	${CMAKE_CURRENT_SOURCE_DIR}/core/vector_equation.h 
	${CMAKE_CURRENT_SOURCE_DIR}/core/function.h 
	${CMAKE_CURRENT_SOURCE_DIR}/core/membrane_property.h
	${CMAKE_CURRENT_SOURCE_DIR}/core/property.h
	${CMAKE_CURRENT_SOURCE_DIR}/core/delay.h
	${CMAKE_CURRENT_SOURCE_DIR}/core/system.h
	${CMAKE_CURRENT_SOURCE_DIR}/core/cpm_sampler.h
)

SET(XSD_SOURCES 
	core/base_types.xsd
	core/diff_eqn.xsd
	core/equation.xsd
	core/field.xsd
	core/function.xsd
	core/lattice.xsd
	core/membrane_property.xsd
	core/property.xsd
	
	core/simulation.xsd
	core/system.xsd
)

IF (Boost_FOUND) 
  LIST(APPEND XSD_SOURCES core/delay.xsd)
  LIST(APPEND APP_DOC_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/core/delay.h)
ENDIF()

foreach(source ${plugin_src})
	STRING(REGEX REPLACE "\\.cpp" ".h" header ${source})
	STRING(REGEX REPLACE "\\.cpp" ".xsd" xsd ${source})
	list(APPEND APP_DOC_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/plugins/${header})
	if ( EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/plugins/${xsd} )
		list(APPEND XSD_SOURCES plugins/${xsd})
	else()
		message(WARNING "no file plugins/${xsd}" )
	endif()
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
		SET(MERGE_OUTPUT "${xsd_file}_merged")
		STRING(REPLACE " " "%20" SCRIPT_SAVE_INTERMEDIATE ${MERGE_INTERMEDIATE})
		SET(SCRIPT_SAVE_INTERMEDIATE \\\"${SCRIPT_SAVE_INTERMEDIATE}\\\")
		ADD_CUSTOM_COMMAND(
			OUTPUT ${MERGE_OUTPUT}
			COMMAND ${XSLTPROC} -o ${MERGE_OUTPUT} --param with ${SCRIPT_SAVE_INTERMEDIATE} ${XML_MERGE_TRANS_SCRIPT} "${CMAKE_CURRENT_SOURCE_DIR}/${xsd_file}"
			DEPENDS ${xsd_file} ${MERGE_INTERMEDIATE} ${XML_MERGE_TRANS_SCRIPT}
			COMMENT "Merging in ${xsd_file} into ${XSD_NAME}"
		)
		SET(MERGE_INTERMEDIATE "${CMAKE_CURRENT_BINARY_DIR}/${MERGE_OUTPUT}")
	ELSE()
		SET(MERGE_INTERMEDIATE ${CMAKE_CURRENT_SOURCE_DIR}/${xsd_file})
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
	xmlSchema ALL
	DEPENDS ${XSD_NAME} ${XSD_SOURCES}
)


SET(APP_DOC_HEADERS ${APP_DOC_HEADERS} PARENT_SCOPE)
SET (MORPHEUS_XSD_FILE ${CMAKE_CURRENT_BINARY_DIR}/${XSD_NAME} PARENT_SCOPE)


