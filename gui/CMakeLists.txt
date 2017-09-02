MESSAGE(STATUS "BUILDING MORPHEUS GUI")
FIND_PACKAGE(Qt4 COMPONENTS QtCore QtGui QtSql QtNetwork QtWebkit QtXML QtHelp QtSvg REQUIRED)
### Include Qt directories  *  Add Qt standard Defines  *  Create QT_LIBRARIES variable, including all required libs 

FIND_PACKAGE(ZLIB REQUIRED)

SET(GUI_EXEC_NAME "morpheus-gui")
#OPTION(MORPHEUS_REMOTE "Enable support for remote computing resources. Depends on libSSH." OFF)

INCLUDE(${QT_USE_FILE})
ADD_DEFINITIONS(${QT_DEFINITIONS})

#IF (MORPHEUS_REMOTE)
#	FIND_PACKAGE(SSH)
#	IF(NOT LIBSSH_FOUND)
#		SET(MORPHEUS_REMOTE OFF FORCE)
#		MESSAGE(STATUS "LIBSSH not found. Remote computing support disabled.")
#	ENDIF()
#ENDIF()

OPTION(LIBSBML_STATIC "Link the libSBML Library statically." OFF)
mark_as_advanced(LIBSBML_STATIC)

If (LIBSBML_STATIC OR MORPHEUS_STATIC_BUILD)
	set(STD_LIB_SUFFIX ${CMAKE_FIND_LIBRARY_SUFFIXES})
	set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
	FIND_PACKAGE(libSBML)
	set(CMAKE_FIND_LIBRARY_SUFFIXES ${STD_LIB_SUFFIX})
ELSE()
	FIND_PACKAGE(libSBML)
ENDIF()

##  check whether the required sqlite plugin is available (if fails if the Variable ends eith NOTFOUND)
if( ( ${CMAKE_VERSION} VERSION_GREATER "2.8.4") AND (NOT ${MORPHEUS_OS} STREQUAL "WIN32") )
	if (NOT QT_QSQLITE_PLUGIN_RELEASE)
		message(FATAL_ERROR "QT SQLITE plugin not found. Install libqt4-sql-sqlite package .. ${MORPHEUS_OS}" )
	ENDIF()
ENDIF()

FIND_PACKAGE(Doxygen REQUIRED)
SET(QT_DOC_FOLDER ${CMAKE_CURRENT_BINARY_DIR}/doc)

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
#message(STATUS "AppDoc header files (${APP_DOC_HEADER_NUM}) are ${APP_DOC_HEADERS}")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/appdoc/morpheus.doxy.in ${CMAKE_CURRENT_BINARY_DIR}/morpheus.doxy @ONLY)

SET(DOXYGEN_EXTRA_FILES  "appdoc/doxygen_header.html" "appdoc/doxygen_footer.html" "appdoc/morpheus_style.css" "appdoc/dynsections.js" "appdoc/jquery.js")


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
	  COMMAND sed -itmp "s/<files>/<files> <file>morpheus\\.png<\\/file><file>menu\\.js<\\/file><file>menudata\\.js<\\/file>/" ${QT_HELP_PROJECT}
	  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/morpheus.doxy ${APP_DOC_HEADERS} ${DOXYGEN_EXTRA_FILES}
	  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	  COMMENT "Generating in-application documentation with Doxygen" VERBATIM
  )
ELSE()
  #message("Doxygen Version ${DOXYGEN_VERSION} is above 1.8.9. Not applying QHP patch.")
  ADD_CUSTOM_COMMAND(
	  OUTPUT ${QT_HELP_PROJECT}
	  COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/morpheus.doxy
	  COMMAND sed -itmp "s/<files>/<files> <file>morpheus\\.png<\\/file><file>menu\\.js<\\/file><file>menudata\\.js<\\/file>/" ${QT_HELP_PROJECT}
	  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/morpheus.doxy ${APP_DOC_HEADERS}  ${DOXYGEN_EXTRA_FILES}
	  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	  COMMENT "Generating in-application documentation with Doxygen" VERBATIM
  )
ENDIF()


configure_file(${CMAKE_CURRENT_SOURCE_DIR}/appdoc/morpheus.qhcp.in ${QT_HELP_COLLECTION_PROJECT} @ONLY)
## File list generator for mathjax.qhp::  find * -type f -exec echo "<file>MathJax/{}</file>" \;
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/appdoc/mathjax.qhp.in ${QT_HELP_MATHJAX_PROJECT} @ONLY)

ADD_CUSTOM_COMMAND(
	OUTPUT ${QT_HELP_COLLECTION} ${QT_HELP_COMPRESSED}
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/appdoc/MathJax ${QT_DOC_FOLDER}/html/MathJax
	COMMAND ${QT_QCOLLECTIONGENERATOR_EXECUTABLE} ${QT_HELP_COLLECTION_PROJECT} -o ${QT_HELP_COLLECTION}
	DEPENDS ${QT_HELP_PROJECT} ${QT_HELP_COLLECTION_PROJECT}  ${QT_HELP_MATHJAX_PROJECT}
	WORKING_DIRECTORY ${QT_DOC_FOLDER}
	COMMENT "Assembling in-application documentation with qcollectiongenerator" VERBATIM
)

add_custom_target( AppDoc DEPENDS ${QT_HELP_COLLECTION} ${QT_HELP_COMPRESSED} )

### set the source, headers and resources
SET( SOURCES
    about_model.cpp
    abstractattribute.cpp 
    abstractprocess.cpp 
    addattrdialog.cpp 
    attrcontroller.cpp 
    config.cpp 
    docu_dock.cpp
    domnodeviewer.cpp
    domnodeeditor.cpp
    equationhighlighter.cpp 
    imagetable.cpp
    infoaction.cpp 
    jobcontroller.cpp
    jobsummary.cpp
    jobview.cpp
    jobviewmodel.cpp 
    job_queue.cpp 
    job_progress_delegate.cpp
    localprocess.cpp 
    main.cpp 
    mainwindow.cpp 
    mathtextedit.cpp 
    model_index_mime_data.cpp
    morpheus_model.cpp
    morpheus_xml.cpp
    moviedialog.cpp
    nodeadapter.cpp
    nodecontroller.cpp 
    parametersweeper.cpp
    paramsweepmodel.cpp
    remoteprocess.cpp 
    settingsdialog.cpp 
    sshproxy.cpp 
    synchronizer.cpp 
    xmlhighlighter.cpp 
    xmlpreview.cpp 
    xsd.cpp 
   )

SET( HEADERS #headers that are commented out are not necessary for moc_* files (avoids compiler warnings)
    abstractattribute.h 
    attrcontroller.h
    addattrdialog.h 
    abstractprocess.h 
    about_model.h
    config.h 
    docu_dock.h
    domnodeeditor.h
    domnodeviewer.h
    equationhighlighter.h 
    imagetable.h
    infoaction.h 
    jobcontroller.h 
    job_progress_delegate.h
    job_queue.h 
    jobviewmodel.h
    jobview.h
    jobsummary.h
    mainwindow.h 
    mathtextedit.h 
    morpheus_model.h
    # morpheus_xml.h
    model_index_mime_data.h
    moviedialog.h 
    nodeadapter.h
    nodecontroller.h
    localprocess.h 
    parametersweeper.h
    paramsweepmodel.h
    remoteprocess.h 
    settingsdialog.h 
    # sshproxy.h
    # synchronizer.h
    # xmlpreview.h
    # xmlhighlighter.h
    # xsd.h
   )

OPTION(MORPHEUS_SBML "Build with SBML import support." ON)

IF(LIBSBML_FOUND AND MORPHEUS_SBML)
	SET(SOURCES ${SOURCES} sbml_converter.cpp)
	SET(HEADERS ${HEADERS} sbml_converter.h)
ENDIF()


## Prepare external ressources for inclusion
# Construct resource list from example files
SET(EXAMPLES_RC_FILES  "")
SET(RESOURCE_DEPENDENCIES "")
foreach(example ${EXAMPLES_SRC} )
	SET(EXAMPLES_RC_FILES  ${EXAMPLES_RC_FILES} "		<file alias=\"${example}\">${EXAMPLES_PATH}/${example}</file>\n")
# 	LIST( APPEND RESOURCE_DEPENDENCIES ${EXAMPLES_PATH}/${example} )
endforeach()
# LIST( APPEND RESOURCE_DEPENDENCIES ${MORPHEUS_XSD_FILE})

# configure examples and XSD intor ext_files.rc
configure_file(ext_files.rc.in ext_files.rc @ONLY)
# compile the resources
SET( RESOURCES link_data.qrc ${CMAKE_CURRENT_BINARY_DIR}/ext_files.rc)
QT4_ADD_RESOURCES( RESOURCES_RCC ${RESOURCES} )

IF( MINGW )
	SET( ICON_RESOURCE icons/win/morpheus.rc )
	ADD_CUSTOM_COMMAND( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/icon.o
		COMMAND windres.exe ARGS -i ${CMAKE_CURRENT_SOURCE_DIR}/${ICON_RESOURCE} -o ${CMAKE_CURRENT_BINARY_DIR}/icon.o
	DEPENDS ${ICON_RESOURCE} )
	SET( SOURCES ${SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/icon.o )
ENDIF( )


### run moc and rcc over the headers and resource files
QT4_WRAP_CPP( HEADERS_MOC ${HEADERS} )

ADD_EXECUTABLE( ${GUI_EXEC_NAME} ${SOURCES} ${HEADERS_MOC} ${RESOURCES_RCC} )
ADD_DEPENDENCIES(${GUI_EXEC_NAME} xmlSchema AppDoc)

IF (MORPHEUS_STATIC_BUILD)
	SET_TARGET_PROPERTIES(${GUI_EXEC_NAME} PROPERTIES LINK_SEARCH_START_STATIC 1)
	SET_TARGET_PROPERTIES(${GUI_EXEC_NAME} PROPERTIES LINK_SEARCH_END_STATIC 1)
ENDIF()

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR} ${ZLIB_INCLUDE_DIR}) 
### create executable and link to Qt libraries and external libraries
# note: use the moc/rcc processed  headers/reources 


TARGET_LINK_LIBRARIES(${GUI_EXEC_NAME} qtsingleapp ${QT_LIBRARIES} ${ZLIB_LIBRARY} )

IF(${MORPHEUS_OS} STREQUAL "WIN32")
	IF( CMAKE_COMPILER_IS_GNUCXX )
		# SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mwindows")
		SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mconsole")
	ENDIF()
	IF(MORPHEUS_STATIC_BUILD)
			TARGET_LINK_LIBRARIES(${GUI_EXEC_NAME} wsock32 ws2_32 )
	ENDIF()
ENDIF()

IF(MORPHEUS_REMOTE)
	INCLUDE_DIRECTORIES(${LIBSSH_INCLUDE_DIR})
	ADD_DEFINITIONS("-DHAVE_LIBSSH")
	TARGET_LINK_LIBRARIES(${GUI_EXEC_NAME} ${LIBSSH_LIBRARIES})
	MESSAGE(STATUS "MORPHEUS GUI is build WITH remote computing support.")
ELSE()
	MESSAGE(STATUS "MORHPEUS GUI is build WITHOUT remote computing support.")
ENDIF()

IF(LIBSBML_FOUND AND MORPHEUS_SBML)
	INCLUDE_DIRECTORIES(${LIBSBML_INCLUDE_DIR})
	
	SET_PROPERTY(TARGET ${GUI_EXEC_NAME} 
				PROPERTY    COMPILE_DEFINITIONS 
				HAVE_LIBSBML
				)
	TARGET_LINK_LIBRARIES(${GUI_EXEC_NAME} ${LIBSBML_LIBRARY})
	IF (LIBSBML_STATIC OR MORPHEUS_STATIC_BUILD) 
		IF (${MORPHEUS_OS} STREQUAL "UNIX")
			find_library(LIBXML2_LIBRARY NAMES xml2)
			TARGET_LINK_LIBRARIES(${GUI_EXEC_NAME} ${LIBXML2_LIBRARY} ${LIBZ_LIBRARY})
		ELSEIF(${MORPHEUS_OS} STREQUAL "WIN32")
			SET_PROPERTY(TARGET ${GUI_EXEC_NAME} 
						PROPERTY    COMPILE_DEFINITIONS 
						LIBSBML_STATIC
						LIBLAX_STATIC
						)
			set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
			find_library(LIBXML_LIBRARY NAMES xml2)
			find_library(LIBICONV_LIBRARY NAMES iconv)
			TARGET_LINK_LIBRARIES(${GUI_EXEC_NAME} ${LIBXML_LIBRARY} ${LIBICONV_LIBRARY})
			TARGET_LINK_LIBRARIES(${GUI_EXEC_NAME} "-lwsock32 -lws2_32" )
		ENDIF()
	ENDIF()

					
	MESSAGE(STATUS "MORPHEUS GUI is build WITH SBML import support.")
ELSE()
	MESSAGE(STATUS "MORPHEUS GUI is build WITHOUT SBML import support.")
ENDIF()


### install the app and the icons
IF( "${MORPHEUS_OS}" STREQUAL "UNIX")
	INSTALL( TARGETS ${GUI_EXEC_NAME} RUNTIME 
			 DESTINATION bin )
	INSTALL( FILES ${QT_HELP_COLLECTION} ${QT_HELP_COMPRESSED} ${QT_HELP_MATHJAX_COMPRESSED}
		DESTINATION share/morpheus
	)
	
	SET( ICONSIZES "16x16" "48x48" "64x64" "128x128")
	SET( IMAGE_PATTERN "morpheus.*" )
	FOREACH(iconsize ${ICONSIZES})
		FILE(GLOB IMAGES "icons/${iconsize}/${IMAGE_PATTERN}")
		INSTALL( FILES ${IMAGES}
				DESTINATION share/icons/hicolor/${iconsize}/apps) 
	ENDFOREACH(iconsize)
	
	find_program(XDG-DESKTOP-MENU_EXECUTABLE xdg-desktop-menu)
	INSTALL( FILES morpheus.desktop 
		DESTINATION share/applications 
	)
	INSTALL(
		CODE "
		execute_process(COMMAND ${XDG-DESKTOP-MENU_EXECUTABLE} install --novendor ${CMAKE_INSTALL_PREFIX}/share/applications/morpheus.desktop) "
	)
	
ELSEIF("${MORPHEUS_OS}" STREQUAL "APPLE")
	INSTALL( TARGETS ${GUI_EXEC_NAME} RUNTIME DESTINATION bin )
	INSTALL( FILES ${QT_HELP_COLLECTION} ${QT_HELP_COMPRESSED}
		DESTINATION share/morpheus
	)
ELSEIF("${MORPHEUS_OS}" STREQUAL "WIN32")
	INSTALL( TARGETS ${GUI_EXEC_NAME} RUNTIME DESTINATION .)
	INSTALL( FILES ${QT_BINARY_DIR}/QtCore4.dll ${QT_BINARY_DIR}/QtGui4.dll ${QT_BINARY_DIR}/QtCLucene4.dll ${QT_BINARY_DIR}/QtHelp4.dll ${QT_BINARY_DIR}/QtNetwork4.dll ${QT_BINARY_DIR}/QtWebKit4.dll ${QT_BINARY_DIR}/QtXml4.dll ${QT_BINARY_DIR}/QtSvg4.dll ${QT_BINARY_DIR}/QtSql4.dll ${QT_BINARY_DIR}/libstdc++-6.dll ${QT_BINARY_DIR}/libgcc_s_dw2-1.dll ${QT_BINARY_DIR}/libwinpthread-1.dll DESTINATION .)
	INSTALL (FILES ${QT_QSQLITE_PLUGIN_RELEASE} DESTINATION sqldrivers )
	INSTALL( FILES ${QT_HELP_COLLECTION} ${QT_HELP_COMPRESSED} ${QT_HELP_MATHJAX_COMPRESSED}
		DESTINATION .
	)
ENDIF()

SET(GUI_EXEC_NAME ${GUI_EXEC_NAME} PARENT_SCOPE)
SET(LIBSBML_FOUND ${LIBSBML_FOUND} PARENT_SCOPE)
