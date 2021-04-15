# IF(MORPHEUS_STATIC_BUILD)
# 	set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
# 	# ich haette lieber am echten Ende der Linkerliste '-Wl,--whole-archive  -lpthread -Wl,--no-whole-archive'
# 	SET(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} "-static -static-libstdc++ -static-libgcc")
# ENDIF()

INCLUDE(CheckIncludeFiles)
INCLUDE(CheckIncludeFileCXX)

CHECK_INCLUDE_FILES("sys/types.h;sys/stat.h;sys/unistd.h" HAVE_GNU_SYSLIB_H)

# Configuration header file for the platform
INCLUDE (CheckTypeSize) 
CHECK_TYPE_SIZE(uint UINT)
# sets HAVE_UINT


###########################
## Target definitions
###########################

ADD_EXECUTABLE(MorpheusSIM "")
SET_TARGET_PROPERTIES(MorpheusSIM PROPERTIES  OUTPUT_NAME ${MORPHEUS_EXEC_NAME} )
IF(HAVE_IPO)
	SET_TARGET_PROPERTIES(MorpheusSIM PROPERTIES  INTERPROCEDURAL_OPTIMIZATION TRUE )
ENDIF()

ADD_LIBRARY(MorpheusCore OBJECT ""  )
TARGET_INCLUDE_DIRECTORIES(MorpheusCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/core)
target_compile_options(MorpheusCore PUBLIC "-mavx")

###############################
## Internal target dependencies
###############################

TARGET_LINK_LIBRARIES_PATCHED(MorpheusSIM PRIVATE MorpheusCore)
TARGET_LINK_LIBRARIES_PATCHED(MorpheusCore PUBLIC muParser xmlParser gnuplot_interface tiny-process-library)

IF(HAVE_OPENMP)
	if (CMAKE_VERSION VERSION_LESS 3.9)
		# set_property( TARGET MorpheusCore APPEND COMPILE_FLAGS ${OpenMP_CXX_FLAGS})
		SET_TARGET_PROPERTIES(MorpheusCore PROPERTIES  COMPILE_FLAGS ${OpenMP_CXX_FLAGS} )
		target_link_libraries_PATCHED(MorpheusCore PUBLIC ${OpenMP_CXX_LIBRARY})
	ELSE()
		TARGET_LINK_LIBRARIES_PATCHED(MorpheusCore PUBLIC OpenMP::OpenMP_CXX)
# 		dump_target_properties(OpenMP::OpenMP_CXX)
	ENDIF()
ENDIF()

################################
## External Package dependencies
################################

if (MORPHEUS_STATIC_BUILD)
	set(Boost_USE_STATIC_LIBS ON)
endif()
FIND_PACKAGE(Boost COMPONENTS program_options REQUIRED)
SET(HAVE_BOOST 1)
TARGET_LINK_LIBRARIES_PATCHED(MorpheusCore PUBLIC Boost::boost Boost::program_options)


FIND_PACKAGE(ZLIB REQUIRED)
FIND_PACKAGE(TIFF REQUIRED)
TARGET_LINK_LIBRARIES_PATCHED(MorpheusCore PUBLIC ${ZLIB_LIBRARY} ${TIFF_LIBRARY} ${TIFF_LIBRARY})

IF(MORPHEUS_STATIC_BUILD)
	## We guess the static library dependencies from the corresponding dynamic library dependencies
	#		message(FATAL_ERROR "Static binary build only supported on *nix systems") 
	IF(UNIX)
		execute_process( COMMAND ldconfig -p 
						 COMMAND grep "libtiff.so "
						 OUTPUT_VARIABLE TIFF_DYN_LIB
		)
		separate_arguments(TIFF_DYN_LIB)
		list(GET TIFF_DYN_LIB -1 TIFF_DYN_LIB)
		string(STRIP ${TIFF_DYN_LIB} TIFF_DYN_LIB)
	# 	message(STATUS "LibTIFF dynamic lib  ${TIFF_DYN_LIB}")
		
		execute_process( COMMAND ldd ${TIFF_DYN_LIB}
						 OUTPUT_VARIABLE TIFF_DEPENDS
		)
		string(REPLACE "\n" ";" TIFF_DEPENDS ${TIFF_DEPENDS})
		SET(LIB_LIST "")
		FOREACH(DEP ${TIFF_DEPENDS})
			string(REPLACE "." ";" DEP ${DEP})
			LIST(GET DEP 0 DEP)
			STRING(STRIP ${DEP} DEP)
			LIST(APPEND LIB_LIST ${DEP})
		ENDFOREACH()
		SET(TIFF_DEPENDS ${LIB_LIST})
	# 	message(STATUS "LibTIFF dependencies ${TIFF_DEPENDS}")
		IF("libjpeg" IN_LIST TIFF_DEPENDS)
			FIND_PACKAGE(JPEG REQUIRED)
			message(STATUS "LibTIFF requires libjpeg")
			TARGET_LINK_LIBRARIES_PATCHED(MorpheusCore PUBLIC ${JPEG_LIBRARY})
		ENDIF()
		IF("liblzma" IN_LIST TIFF_DEPENDS)
			FIND_PACKAGE(LibLZMA REQUIRED) # newer ubuntu versions
			message(STATUS "LibTIFF requires liblzma")
			TARGET_LINK_LIBRARIES_PATCHED(MorpheusCore PUBLIC ${LIBLZMA_LIBRARIES})
		ENDIF()
		IF("libjbig" IN_LIST TIFF_DEPENDS)
			message(STATUS "LibTIFF requires libjbig")
			TARGET_LINK_LIBRARIES_PATCHED(MorpheusCore PUBLIC -ljbig )
		ENDIF()
		IF("libwebp" IN_LIST TIFF_DEPENDS)
			message(STATUS "LibTIFF requires webp")
			TARGET_LINK_LIBRARIES_PATCHED(MorpheusCore PUBLIC -lwebp )
		ENDIF()
		IF("libzstd" IN_LIST TIFF_DEPENDS)
			message(STATUS "LibTIFF requires zstd")
			TARGET_LINK_LIBRARIES_PATCHED(MorpheusCore PUBLIC -lzstd )
		ENDIF()
	ELSEIF(WIN32)
		MESSAGE(STATUS " WIN32 using MSYS: ${MSYS}")
		# LibTIFF requires libjpeg
		FIND_PACKAGE(JPEG REQUIRED)
		TARGET_LINK_LIBRARIES_PATCHED(MorpheusCore PUBLIC ${JPEG_LIBRARY})
		# LibTIFF requires liblzma
		FIND_PACKAGE(LibLZMA REQUIRED) # newer ubuntu versions
		TARGET_LINK_LIBRARIES_PATCHED(MorpheusCore PUBLIC ${LIBLZMA_LIBRARIES})
		# LibTIFF requires zstd
		TARGET_LINK_LIBRARIES_PATCHED(MorpheusCore PUBLIC -lzstd )
	ENDIF()
ENDIF()


IF(MORPHEUS_GRAPHVIZ STREQUAL "Library")
	SET(GRAPHVIZ_ROOT "" CACHE PATH "Set root directory to search for GraphViz" )
	MARK_AS_ADVANCED(GRAPHVIZ_ROOT)
	FIND_PACKAGE(Graphviz)
ENDIF()

IF(GRAPHVIZ_FOUND)
	target_compile_definitions(MorpheusCore PUBLIC HAVE_GRAPHVIZ=1)
# 	FIND_PACKAGE(EXPAT REQUIRED)
	IF (EXISTS "${GRAPHVIZ_GRAPH_LIBRARY}")
		TARGET_LINK_LIBRARIES_PATCHED(MorpheusCore PUBLIC ${GRAPHVIZ_GVC_LIBRARY} ${GRAPHVIZ_GRAPH_LIBRARY} ${GRAPHVIZ_CDT_LIBRARY} ${GRAPHVIZ_PATHPLAN_LIBRARY})
	ELSE()
		TARGET_LINK_LIBRARIES_PATCHED(MorpheusCore PUBLIC ${GRAPHVIZ_GVC_LIBRARY} ${GRAPHVIZ_CGRAPH_LIBRARY} ${GRAPHVIZ_CDT_LIBRARY} ${GRAPHVIZ_PATHPLAN_LIBRARY})
	ENDIF()
ENDIF()

IF (MORPHEUS_TESTS)
	add_subdirectory(testing)
ENDIF()

add_subdirectory(plugins)
add_subdirectory(core)

IF(MORPHEUS_STATIC_BUILD)
	target_link_libraries_PATCHED(MorpheusCore PUBLIC -Wl,--whole-archive -lpthread -Wl,--no-whole-archive)
# 	TARGET_LINK_LIBRARIES_PATCHED(MorpheusCore PRIVATE -static-libstdc++ -static-libgcc)
ENDIF()

IF  ("${MORPHEUS_OS}" STREQUAL "WIN32")
	# TARGET_LINK_LIBRARIES_PATCHED(MorpheusSIM pthread psapi)

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

# dump_target_properties(MorpheusCore)
# Installation of the program

IF  ( WIN32 )
	INSTALL(TARGETS MorpheusSIM RUNTIME DESTINATION . COMPONENT Morpheus )
	
	Find_PROGRAM(GNUPLOT_EXECUTABLE NAMES gnuplot.exe HINTS ${GNUPLOT_ROOT}/bin ENV GNUPLOT_ROOT REQUIRED)
	INSTALL(PROGRAMS ${GNUPLOT_EXECUTABLE}
		DESTINATION . 
		COMPONENT Morpheus
		EXCLUDE_FROM_ALL)
	
	set(APPS "\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${MORPHEUS_BUNDLE_PREFIX}")
	
	# Directories to look for dependencies
	set(DIRS "${CMAKE_BINARY_DIR}")
	# Path used for searching by FIND_XXX(), with appropriate suffixes added
	LIST(APPEND CMAKE_PREFIX_PATH "C:/msys64/mingw64")
	if(CMAKE_PREFIX_PATH)
		foreach(dir ${CMAKE_PREFIX_PATH})
			list(APPEND DIRS "${dir}/bin" "${dir}/lib")
		endforeach()
	endif()
	#Append Qt's lib folder which is two levels above Qt5Widgets_DIR
	list(APPEND DIRS "${Qt5Widgets_DIR}/../..")
	
	INSTALL(CODE "
		SET(EXECUTABLES \"${APPS}/morpheus.exe;${APPS}/gnuplot.exe\")
		#MESSAGE(\"CPACK: - Searching binaries in ${APPS}/..\")
		message(\"CPACK:  - APPS: \${EXECUTABLES}\")
		message(\"CPACK:  - DIRS: ${DIRS}\")
		file(GET_RUNTIME_DEPENDENCIES
			RESOLVED_DEPENDENCIES_VAR deps_resolved
			UNRESOLVED_DEPENDENCIES_VAR deps_unresolved
			EXECUTABLES \${EXECUTABLES}
			DIRECTORIES ${DIRS})
		foreach(dep \${deps_resolved})
			if( dep MATCHES \"C:[/\\\\]Windows.*\" )
				# do not install system libraries
			else()
				MESSAGE(\"CPACK:  - adding \${dep}\")
				file(INSTALL \"\${dep}\"
				DESTINATION \"${APPS}/${INSTALL_RUNTIME_DIR}\")
			endif()
		endforeach()
		# MESSAGE(\"CPACK:  - Resolved dependencies \${deps_resolved}\")
		# MESSAGE(\"CPACK:  - Unresolved dependencies \${deps_unresolved}\")
		"
		COMPONENT Morpheus
		EXCLUDE_FROM_ALL
	)
	
		
ELSEIF(APPLE)
	IF (MORPHEUS_RELEASE_BUNDLE)
		SET(INSTALL_RUNTIME_DIR "${MORPHEUS_BUNDLE_DIR}/Contents/MacOS" )
		INSTALL(TARGETS MorpheusSIM 
			RUNTIME DESTINATION ${INSTALL_RUNTIME_DIR}
			COMPONENT Morpheus 
		)
		Find_PROGRAM(GNUPLOT_EXECUTABLE NAMES gnuplot HINTS ENV GNUPLOT REQUIRED)
		IF(IS_SYMLINK ${GNUPLOT_EXECUTABLE})
			SET(GNUPLOT_SYMEX ${GNUPLOT_EXECUTABLE})
			FILE(READ_SYMLINK ${GNUPLOT_EXECUTABLE} GNUPLOT_EXECUTABLE)
			IF(NOT IS_ABSOLUTE ${GNUPLOT_EXECUTABLE})
				get_filename_component(dir "${GNUPLOT_SYMEX}" DIRECTORY)
				set(GNUPLOT_EXECUTABLE "${dir}/${GNUPLOT_EXECUTABLE}")
			endif()
		ENDIF()
		INSTALL(PROGRAMS ${GNUPLOT_EXECUTABLE} 
			DESTINATION ${INSTALL_RUNTIME_DIR} 
			COMPONENT Morpheus_SC
			EXCLUDE_FROM_ALL)

	ELSE()
		INSTALL(TARGETS MorpheusSIM RUNTIME DESTINATION bin COMPONENT Morpheus )
	ENDIF()
ELSE()
	INSTALL(TARGETS MorpheusSIM RUNTIME DESTINATION bin COMPONENT Morpheus )
	
ENDIF()



#####################################
##  Docu and Language definition  ##
#####################################

SET(XSD_SOURCES 
	${CMAKE_CURRENT_SOURCE_DIR}/core/base_types.xsd
	${CMAKE_CURRENT_SOURCE_DIR}/core/simulation.xsd
	${CMAKE_CURRENT_SOURCE_DIR}/core/interaction_energy.xsd
)

SET(APP_DOC_HEADERS 
	${CMAKE_CURRENT_SOURCE_DIR}/core/docu.h
	${CMAKE_CURRENT_SOURCE_DIR}/core/diffusion.h
	${CMAKE_CURRENT_SOURCE_DIR}/core/cpm_sampler.h
	${CMAKE_CURRENT_SOURCE_DIR}/core/interaction_energy.h
	${CMAKE_CURRENT_SOURCE_DIR}/core/expression_evaluator.h
)

get_target_property(sources MorpheusCore SOURCES)

foreach(source ${sources})
	STRING(REGEX REPLACE "\\.cpp$" ".h" header ${source})
	STRING(REGEX REPLACE "\\.cpp$" ".xsd" xsd ${source})
	#  check the header for the PLUGIN macro
	file(STRINGS "${header}" lines REGEX "DECLARE_PLUGIN")
	if (lines)
		list(APPEND APP_DOC_HEADERS ${header})
		if ( EXISTS ${xsd} )
			list(APPEND XSD_SOURCES ${xsd})
		else()
			message(WARNING "Missing plugin language description ${xsd}" )
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


