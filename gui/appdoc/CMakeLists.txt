### CREATE IN APP DOCUMENTATION ###
# uses doxygen and qcollectiongenerator
FIND_PACKAGE(Doxygen REQUIRED)

SET(QT_DOC_FOLDER ${CMAKE_CURRENT_BINARY_DIR})

SET(QT_HELP_PROJECT_DOXY "${QT_DOC_FOLDER}/html/index.qhp")
SET(QT_HELP_PROJECT "${QT_DOC_FOLDER}/html/morpheus.qhp")
SET(QT_HELP_COMPRESSED "${QT_DOC_FOLDER}/morpheus.qch")
SET(QT_HELP_MATHJAX_PROJECT "${QT_DOC_FOLDER}/html/mathjax.qhp")
SET(QT_HELP_MATHJAX_COMPRESSED "${QT_DOC_FOLDER}/mathjax.qch")
SET(QT_HELP_COLLECTION_PROJECT "${QT_DOC_FOLDER}/morpheus.qhcp")
SET(QT_HELP_COLLECTION "${QT_DOC_FOLDER}/morpheus.qhc")

IF (CMAKE_VERBOSE_MAKEFILE) 
	SET(CMAKE_QUIET_MAKEFILE NO)
ELSE()
	SET(CMAKE_QUIET_MAKEFILE YES)
ENDIF()

OPTION(DOXYGEN_QHP_PATCH "Patch the QHP File created via doxygen to cure the \"Opening and ending tag mismatch.\" bug." OFF)
mark_as_advanced(DOXYGEN_QHP_PATCH)

# Create a space separated file list for Doxygen
string_join( " " DOXYGEN_SOURCES ${APP_DOC_HEADERS})

FILE( GLOB_RECURSE MATHJAX_SOURCES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "MathJax/*")

SET(HTML_EXTRA_FILE_LIST "dynsections.js" "jquery.js" ${MATHJAX_SOURCES})
string_join(" " HTML_EXTRA_FILES ${HTML_EXTRA_FILE_LIST})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/morpheus.doxy.in ${CMAKE_CURRENT_BINARY_DIR}/morpheus.doxy @ONLY)

SET(DOXYGEN_EXTRA_FILES  "doxygen_header.html" "doxygen_footer.html" "morpheus_style.css" ${HTML_EXTRA_FILE_LIST})


IF (DOXYGEN_QHP_PATCH)
	## HERE we delete one line that is mistakenly inserted by doxygen into the QT_HELP_PROJECT
	## The line number has to be adjusted according to the structure of the Documentation !!
	message(STATUS "Applying Doxygen QHP patch.")
	ADD_CUSTOM_COMMAND(
		OUTPUT ${QT_HELP_PROJECT_DOXY}
		COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/morpheus.doxy
		COMMAND ${CMAKE_COMMAND} -E rename ${QT_HELP_PROJECT} ${QT_HELP_PROJECT_DOXY}.tmp
		COMMAND awk "BEGIN{lvl=0; occ=0;} { closing=0; if (/<section.+\\/>/) lvl+=0; else if (/<section/) lvl++; else  if (/<\\/section/) { closing=1; lvl--;} if ( !(lvl==0 && closing==1 && occ==0) ) {print \$0} else occ=1; }"  ${QT_HELP_PROJECT_DOXY}.tmp > ${QT_HELP_PROJECT_DOXY}
		COMMAND ${CMAKE_COMMAND} -E remove ${QT_HELP_PROJECT_DOXY}.tmp
		DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/morpheus.doxy ${APP_DOC_HEADERS} ${DOXYGEN_EXTRA_FILES}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		COMMENT "Generating in-application documentation with Doxygen" VERBATIM
	)
ELSE()
	ADD_CUSTOM_COMMAND(
		OUTPUT ${QT_HELP_PROJECT_DOXY}
		COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/morpheus.doxy
		DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/morpheus.doxy ${APP_DOC_HEADERS}  ${DOXYGEN_EXTRA_FILES}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		COMMENT "Generating in-application documentation with Doxygen" VERBATIM
	)
ENDIF()

SET(QHP_EXTRA_FILES ${MATHJAX_SOURCE})
IF( "${DOXYGEN_VERSION}" STREQUAL "1.8.11")
	LIST(APPEND QHP_EXTRA_FILES "dynsections.js" "jquery.js")
ENDIF()

configure_file(patch_qhp.cmake.in patch_qhp.cmake @ONLY)
ADD_CUSTOM_COMMAND(
	OUTPUT ${QT_HELP_PROJECT}
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/MathJax ${QT_DOC_FOLDER}/html/MathJax
	COMMAND ${CMAKE_COMMAND} -DPROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}" -P "patch_qhp.cmake"
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	DEPENDS ${QT_HELP_PROJECT_DOXY} ${QHP_EXTRA_FILES}  ${CMAKE_CURRENT_BINARY_DIR}/patch_qhp.cmake
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/morpheus.qhcp.in ${QT_HELP_COLLECTION_PROJECT} @ONLY)


ADD_CUSTOM_COMMAND(
	OUTPUT ${QT_HELP_COLLECTION} ${QT_HELP_COMPRESSED}
	COMMAND $<TARGET_FILE:Qt5::qhelpgenerator> ${QT_HELP_COLLECTION_PROJECT} -o ${QT_HELP_COLLECTION}
	DEPENDS ${QT_HELP_PROJECT} ${QT_HELP_COLLECTION_PROJECT}
	WORKING_DIRECTORY ${QT_DOC_FOLDER}
	COMMENT "Assembling in-application documentation with qcollectiongenerator" VERBATIM
)


FIND_PACKAGE(Doxygen REQUIRED)
ADD_CUSTOM_TARGET(AppDoc
	  DEPENDS ${QT_HELP_COLLECTION} ${QT_HELP_COMPRESSED}
	  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	  COMMENT "Generating documentation with Doxygen" VERBATIM )

set_target_properties(AppDoc PROPERTIES RESOURCE "${QT_HELP_COLLECTION};${QT_HELP_COMPRESSED}")
