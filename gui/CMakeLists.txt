MESSAGE(STATUS "BUILDING MORPHEUS GUI")

IF(POLICY CMP0071)
	cmake_policy(SET CMP0071 NEW)
ENDIF()

SET(MORPHEUS_WEBVIEW "Auto" CACHE STRING "Manually select the webview to be used (Auto,RTF,WebKit,WebEngine)")
SET_PROPERTY(CACHE MORPHEUS_WEBVIEW PROPERTY STRINGS Auto RTF WebKit WebEngine)

OPTION(MORPHEUS_SBML "Build with SBML import support." ON)

OPTION(LIBSBML_STATIC "Link the libSBML Library statically." OFF)
mark_as_advanced(LIBSBML_STATIC)


### Include Qt directories  *  Add Qt standard Defines  *  Create QT_LIBRARIES variable, including all required libs 

######################################
## Select the proper WebView        ##
######################################

FIND_PACKAGE(Qt5 COMPONENTS Widgets Sql Network Xml Svg Help)

IF ( "${MORPHEUS_WEBVIEW}" STREQUAL "Auto" )
	FIND_PACKAGE(Qt5 QUIET OPTIONAL_COMPONENTS WebEngineWidgets)
	IF ( TARGET Qt5::WebEngineWidgets )
		SET(USE_QWebEngine TRUE)
	ELSE()
		FIND_PACKAGE(Qt5 QUIET OPTIONAL_COMPONENTS WebKitWidgets)
		IF ( TARGET Qt5::WebKitWidgets )
			SET(USE_QWebKit TRUE)
		ELSE()
			MESSAGE(WARNING Could not find Qt'sWebEngineView and WebKitView. Falling back to Text mode.)
			MESSAGE(WARNING This is likely not what you want.)
			SET(USE_QTextBrowser TRUE)
		ENDIF()
	ENDIF()
ELSEIF ("${MORPHEUS_WEBVIEW}" STREQUAL "WebEngine")
	FIND_PACKAGE(Qt5 COMPONENTS WebEngineWidgets)
	SET(USE_QWebEngine TRUE)	
ELSEIF ("${MORPHEUS_WEBVIEW}" STREQUAL "WebKit")
	FIND_PACKAGE(Qt5 COMPONENTS WebKitWidgets)
	SET(USE_QWebKit TRUE)
ELSE()
	SET(USE_QTextBrowser TRUE)
ENDIF()

######################################
## Find or Build SBML               ##
######################################

IF ( NOT BUILD_LIBSBML )
    # Find on platform
    message(STATUS "Find libSBML on the platform.")
    If (LIBSBML_STATIC OR MORPHEUS_STATIC_BUILD)
        set(STD_LIB_SUFFIX ${CMAKE_FIND_LIBRARY_SUFFIXES})
        set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_STATIC_LIBRARY_SUFFIX})
        FIND_PACKAGE(libSBML)
        set(CMAKE_FIND_LIBRARY_SUFFIXES ${STD_LIB_SUFFIX})
    ELSE()
        FIND_PACKAGE(libSBML)
    ENDIF()
ENDIF()

# Build locally
IF ( NOT LIBSBML_FOUND OR BUILD_LIBSBML )
    message(STATUS "Building libSBML internally ...")
    SET(BUILD_LIBSBML TRUE CACHE BOOL "Build libSBML from source")
    add_subdirectory(${CMAKE_SOURCE_DIR}/3rdparty/libSBML ${CMAKE_BINARY_DIR}/3rdparty/libSBML)
    SET(LIBSBML_FOUND TRUE CACHE BOOL "libSBML found" FORCE)
    # TODO ADD libxml2 to be a package dependency or to bundle the library
    SET(LIBSBML_STATIC TRUE CACHE STRING "Link the libSBML Library statically." FORCE)
    If (LIBSBML_STATIC)
        SET(LIBSBML_LIBRARY ${LIBSBML_STATIC_LIBRARIES} CACHE STRING "libSBML libraries" FORCE) 
    ELSE() 
        SET(LIBSBML_LIBRARY ${LIBSBML_SHARED_LIBRARIES} CACHE STRING "libSBML libraries" FORCE) 
        # TODO Also install the  LIBSBML_SHARED_LIBRARIES
    ENDIF()
ELSE()
    SET(BUILD_LIBSBML FALSE CACHE BOOL "Build libSBML from source")
ENDIF()


#
# ##  check whether the required sqlite plugin is available (if fails if the Variable ends eith NOTFOUND)
# if( ( CMAKE_VERSION VERSION_GREATER "2.8.4") AND (NOT "${MORPHEUS_OS}" STREQUAL "WIN32") )
# 	if (NOT QT_QSQLITE_PLUGIN_RELEASE)
# 		message(FATAL_ERROR "QT SQLITE plugin not found. Install libqt4-sql-sqlite package .. ${MORPHEUS_OS}" )
# 	ENDIF()
#ENDIF()

IF(LIBSBML_FOUND AND MORPHEUS_SBML)
	SET(SOURCES ${SOURCES} sbml_converter.cpp)
ENDIF()

##################################
##  Other dependencies          ##
##################################

FIND_PACKAGE(ZLIB REQUIRED)

#OPTION(MORPHEUS_REMOTE "Enable support for remote computing resources. Depends on libSSH." OFF)


#IF (MORPHEUS_REMOTE)
#	FIND_PACKAGE(SSH)
#	IF(NOT LIBSSH_FOUND)
#		SET(MORPHEUS_REMOTE OFF FORCE)
#		MESSAGE(STATUS "LIBSSH not found. Remote computing support disabled.")
#	ENDIF()
#ENDIF()

##  check whether the required sqlite plugin is available (if fails if the Variable ends eith NOTFOUND)
# if( ( CMAKE_VERSION VERSION_GREATER "2.8.4") AND (NOT "${MORPHEUS_OS}" STREQUAL "WIN32") )
# 	if (NOT QT_QSQLITE_PLUGIN_RELEASE)
# 		message(FATAL_ERROR "QT SQLITE plugin not found. Install libqt4-sql-sqlite package .. ${MORPHEUS_OS}" )
# 	ENDIF()
# ENDIF()



### set the source, headers and resources
SET( SOURCES ${SOURCES}
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
    announcement.cpp
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
    network_access.cpp
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
    widgets/checkboxlist.cpp
    widgets/webviewer.cpp
   )

IF ( USE_QWebEngine )
	SET( SOURCES ${SOURCES} network_schemes.cpp )
ENDIF()


#################################################
## Prepare external ressources for inclusion   ##
#################################################

# Construct resource list from example files
SET(EXAMPLES_FILES ${EXAMPLES_SRC})
IF (CMAKE_VERSION VERSION_LESS "3.12")
	SET(EXAMPLES_RC_FILES "")
	FOREACH( EXAMPLE ${EXAMPLES_FILES} )
		STRING(APPEND EXAMPLES_RC_FILES "              <file alias=\"${EXAMPLE}\">${EXAMPLES_PATH}/${EXAMPLE}</file>\n" )
	ENDFOREACH()
ELSE()
	LIST(TRANSFORM EXAMPLES_FILES REPLACE "(.+)" "           <file alias=\"\\1\">${EXAMPLES_PATH}/\\1</file>")
	LIST(JOIN EXAMPLES_FILES "\n" EXAMPLES_RC_FILES)
ENDIF()

configure_file(ext_files.rc.in ext_files.rc @ONLY)

# compile the resources
SET( RESOURCES link_data.qrc ${CMAKE_CURRENT_BINARY_DIR}/ext_files.rc)
IF(MORPHEUS_GRAPHVIZ STREQUAL "WebRenderer")
	LIST(APPEND RESOURCES graphviz_renderer.qrc)
	set_property(SOURCE about_model.cpp APPEND PROPERTY COMPILE_DEFINITIONS GRAPHVIZ_WEB_RENDERER=1 )
ENDIF()

QT5_ADD_RESOURCES( RESOURCES_RCC ${RESOURCES} OPTIONS "--compress" "8" )


IF( MINGW )
	SET( ICON_RESOURCE icons/win/morpheus.rc )
	ADD_CUSTOM_COMMAND( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/icon.o
		COMMAND windres.exe ARGS -i ${CMAKE_CURRENT_SOURCE_DIR}/${ICON_RESOURCE} -o ${CMAKE_CURRENT_BINARY_DIR}/icon.o
	DEPENDS ${ICON_RESOURCE} )
	SET( SOURCES ${SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/icon.o )
ENDIF( )


### run moc and rcc over the headers and resource files
SET(CMAKE_AUTOMOC ON)
SET(CMAKE_INCLUDE_CURRENT_DIR ON)


ADD_EXECUTABLE( MorpheusGUI WIN32 MACOSX_BUNDLE ${SOURCES} ${RESOURCES_RCC} )
set_target_properties(MorpheusGUI PROPERTIES RUNTIME_OUTPUT_NAME ${GUI_EXEC_NAME})
add_subdirectory(appdoc)

ADD_DEPENDENCIES(MorpheusGUI xmlSchema AppDoc)

# IF (MORPHEUS_STATIC_BUILD)
# 	SET_TARGET_PROPERTIES(${GUI_EXEC_NAME} PROPERTIES LINK_SEARCH_START_STATIC 1)
# 	SET_TARGET_PROPERTIES(${GUI_EXEC_NAME} PROPERTIES LINK_SEARCH_END_STATIC 1)
# ENDIF()

INCLUDE_DIRECTORIES( ${ZLIB_INCLUDE_DIR}) 

### create executable and link to Qt libraries and external libraries
# qt5_use_modules(MorpheusGUI Widgets Xml Sql Svg Help Network)

TARGET_LINK_LIBRARIES(MorpheusGUI Qt5::Widgets Qt5::Xml Qt5::Sql Qt5::Svg Qt5::Help Qt5::Network qtsingleapp ${ZLIB_LIBRARY} )

IF( "${MORPHEUS_OS}" STREQUAL "WIN32")
	IF( CMAKE_COMPILER_IS_GNUCXX )
		# SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mwindows")
		SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mconsole")
	ENDIF()
	IF(MORPHEUS_STATIC_BUILD)
		TARGET_LINK_LIBRARIES(MorpheusGUI wsock32 ws2_32 )
	ENDIF()
ENDIF()

IF ( USE_QWebEngine )
	MESSAGE(STATUS "MORHPEUS GUI is build WITH QWebEngine.")
	ADD_DEFINITIONS(-DUSE_QWebEngine)
	TARGET_LINK_LIBRARIES(MorpheusGUI Qt5::WebEngineWidgets)
ELSEIF( USE_QWebKit)
	MESSAGE(STATUS "MORHPEUS GUI is build WITH QtWebKit.")
	ADD_DEFINITIONS(-DUSE_QWebKit)
	TARGET_LINK_LIBRARIES(MorpheusGUI Qt5::WebKitWidgets)
ELSE()
	MESSAGE(STATUS "MORHPEUS GUI is build WITH QTextBrowser.")
	ADD_DEFINITIONS(-DUSE_QTextBrowser)
ENDIF()

IF(LIBSBML_FOUND AND MORPHEUS_SBML)
	INCLUDE_DIRECTORIES(${LIBSBML_INCLUDE_DIR})
	ADD_DEFINITIONS(-DHAVE_LIBSBML)
    ADD_DEFINITIONS( ${LIBSBML_DEFINITIONS} )
	TARGET_LINK_LIBRARIES(MorpheusGUI ${LIBSBML_LIBRARY})
	IF(BUILD_LIBSBML)
        ADD_DEPENDENCIES(MorpheusGUI libSBML)
	ENDIF()
	
# 	IF (LIBSBML_STATIC OR MORPHEUS_STATIC_BUILD) 
# 		MESSAGE(STATUS "Searching SBML dependencies for static linking")
# 		
# 		IF ( "${MORPHEUS_OS}" STREQUAL "UNIX")
# 			TARGET_LINK_LIBRARIES(${GUI_EXEC_NAME}  ${LIBZ_LIBRARY} ${LIBXML2_LIBRARY} )
# 		ELSEIF("${MORPHEUS_OS}" STREQUAL "WIN32")
# 		    MESSAGE(STATUS "Adding Windows SBML dependencies")
# 			ADD_DEFINITIONS( -DLIBSBML_STATIC)
# 			set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
# 
# 			TARGET_LINK_LIBRARIES(${GUI_EXEC_NAME} ${LIBXML_LIBRARY} ${LIBICONV_LIBRARY} ${LIBCHARSET_LIBRARY})
# 			TARGET_LINK_LIBRARIES(${GUI_EXEC_NAME} "-lwsock32 -lws2_32" )
# 		ENDIF()
# 	ENDIF()

					
	MESSAGE(STATUS "MORPHEUS GUI is build WITH SBML import support.")
ELSE()
	MESSAGE(STATUS "MORPHEUS GUI is build WITHOUT SBML import support.")
ENDIF()

IF(MORPHEUS_REMOTE)
	INCLUDE_DIRECTORIES(${LIBSSH_INCLUDE_DIR})
	ADD_DEFINITIONS("-DHAVE_LIBSSH")
	TARGET_LINK_LIBRARIES(MorpheusGUI ${LIBSSH_LIBRARIES})
	MESSAGE(STATUS "MORPHEUS GUI is build WITH remote computing support.")
ELSE()
	MESSAGE(STATUS "MORHPEUS GUI is build WITHOUT remote computing support.")
ENDIF()


### install the app and the icons
IF( "${MORPHEUS_OS}" STREQUAL "UNIX")
	INSTALL( TARGETS MorpheusGUI RUNTIME 
			 DESTINATION bin )
	
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
	INSTALL( TARGETS MorpheusGUI RUNTIME DESTINATION bin )
ELSEIF("${MORPHEUS_OS}" STREQUAL "WIN32")
	INSTALL( TARGETS MorpheusGUI RUNTIME DESTINATION .)
	find_library(LIB_WINPTHREAD "libwinpthread-1" PATHS ${QT_BINARY_DIR} )
	INSTALL( FILES ${QT_BINARY_DIR}/QtCore4.dll ${QT_BINARY_DIR}/QtGui4.dll ${QT_BINARY_DIR}/QtCLucene4.dll ${QT_BINARY_DIR}/QtHelp4.dll ${QT_BINARY_DIR}/QtNetwork4.dll ${QT_BINARY_DIR}/QtWebKit4.dll ${QT_BINARY_DIR}/QtXml4.dll ${QT_BINARY_DIR}/QtSvg4.dll ${QT_BINARY_DIR}/QtSql4.dll ${LIB_WINPTHREAD} DESTINATION .)
	if ( NOT ${LIBSBML_STATIC} ) 
		GET_FILENAME_COMPONENT(LIBSBML_LIB_PATH ${LIBSBML_LIBRARY} DIRECTORY)
		INSTALL( FILES ${LIBSBML_LIB_PATH}/../bin/libsbml.dll  ${LIBSBML_LIB_PATH}/../bin/libxml2.dll DESTINATION .)
	ENDIF()
	INSTALL (FILES ${QT_QSQLITE_PLUGIN_RELEASE} DESTINATION sqldrivers )
ENDIF()

SET(LIBSBML_FOUND ${LIBSBML_FOUND} PARENT_SCOPE)
