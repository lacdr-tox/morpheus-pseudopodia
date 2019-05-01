### CREATE IN APP DOCUMENTATION ###
# uses doxygen and qcollectiongenerator

SET(QT_DOC_FOLDER ${CMAKE_CURRENT_BINARY_DIR})

SET(QT_HELP_PROJECT "${QT_DOC_FOLDER}/html/index.qhp")
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
#The conditions for which the doxygen qhp patch has to be applied is still unclear
# IF (DOXYGEN_VERSION VERSION_LESS "1.8.9")
# 	SET(DOXYGEN_QHP_PATCH ON)
# ENDIF()


# Create a space separated file list for Doxygen
SET(DOXYGEN_SOURCES "" )
foreach(source ${APP_DOC_HEADERS})
	SET(DOXYGEN_SOURCES "${DOXYGEN_SOURCES} ${source}")
endforeach()

get_target_property(AppDoc SOURCE APP_DOC_HEADERS)
SET(DOXYGEN_SOURCES "${DOXYGEN_SOURCES} ${APP_DOC_HEADERS}")
#message(STATUS "AppDoc header files (${APP_DOC_HEADER_NUM}) are ${APP_DOC_HEADERS}")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/morpheus.doxy.in ${CMAKE_CURRENT_BINARY_DIR}/morpheus.doxy @ONLY)

SET(DOXYGEN_EXTRA_FILES  "doxygen_header.html" "doxygen_footer.html" "morpheus_style.css" "dynsections.js" "jquery.js")


IF (DOXYGEN_QHP_PATCH)
  ## HERE we delete one line that is mistakenly inserted by doxygen into the QT_HELP_PROJECT
  ## The line number has to be adjusted according to the structure of the Documentation !!
  message(STATUS "Applying Doxygen QHP patch.")
  ADD_CUSTOM_COMMAND(
	  OUTPUT ${QT_HELP_PROJECT}
	  COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/morpheus.doxy
	  COMMAND ${CMAKE_COMMAND} -E rename ${QT_HELP_PROJECT} ${QT_HELP_PROJECT}.tmp
	  COMMAND awk "BEGIN{lvl=0; occ=0;} { closing=0; if (/<section.+\\/>/) lvl+=0; else if (/<section/) lvl++; else  if (/<\\/section/) { closing=1; lvl--;} if ( !(lvl==0 && closing==1 && occ==0) ) {print \$0} else occ=1; }"  ${QT_HELP_PROJECT}.tmp > ${QT_HELP_PROJECT}
	  COMMAND ${CMAKE_COMMAND} -E remove ${QT_HELP_PROJECT}.tmp
	  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/morpheus.doxy ${APP_DOC_HEADERS} ${DOXYGEN_EXTRA_FILES}
	  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	  COMMENT "Generating in-application documentation with Doxygen" VERBATIM
  )
ELSEIF( "${DOXYGEN_VERSION}" STREQUAL "1.8.11")
  ADD_CUSTOM_COMMAND(
	  OUTPUT ${QT_HELP_PROJECT}
	  COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/morpheus.doxy
	  COMMAND sed -itmp "s/<files>/<files><file>dynsections\\.js<\\/file> <file>morpheus\\.png<\\/file>/" ${QT_HELP_PROJECT} 
	  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/morpheus.doxy ${APP_DOC_HEADERS}  ${DOXYGEN_EXTRA_FILES}
	  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	  COMMENT "Generating in-application documentation with Doxygen" VERBATIM
  )
ELSE()  
  ADD_CUSTOM_COMMAND(
	  OUTPUT ${QT_HELP_PROJECT}
	  COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/morpheus.doxy
	  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/morpheus.doxy ${APP_DOC_HEADERS}  ${DOXYGEN_EXTRA_FILES}
	  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	  COMMENT "Generating in-application documentation with Doxygen" VERBATIM
  )
ENDIF()


configure_file(${CMAKE_CURRENT_SOURCE_DIR}/morpheus.qhcp.in ${QT_HELP_COLLECTION_PROJECT} @ONLY)
## File list generator for mathjax.qhp::  find * -type f -exec echo "<file>MathJax/{}</file>" \;
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/mathjax.qhp.in ${QT_HELP_MATHJAX_PROJECT} @ONLY)

ADD_CUSTOM_COMMAND(
	OUTPUT ${QT_HELP_COLLECTION} ${QT_HELP_COMPRESSED}
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/MathJax ${QT_DOC_FOLDER}/html/MathJax
	COMMAND ${QT_QCOLLECTIONGENERATOR_EXECUTABLE} ${QT_HELP_COLLECTION_PROJECT} -o ${QT_HELP_COLLECTION}
	DEPENDS ${QT_HELP_PROJECT} ${QT_HELP_COLLECTION_PROJECT}  ${QT_HELP_MATHJAX_PROJECT}
	WORKING_DIRECTORY ${QT_DOC_FOLDER}
	COMMENT "Assembling in-application documentation with qcollectiongenerator" VERBATIM
)

add_dependencies(AppDoc ${QT_HELP_COLLECTION} ${QT_HELP_COMPRESSED} )

