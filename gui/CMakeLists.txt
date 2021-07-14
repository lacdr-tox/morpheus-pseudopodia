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

FIND_PACKAGE(Qt5 COMPONENTS Widgets Sql Network Concurrent Xml Svg Help)

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
SET(LIBSBML_FOUND FALSE)

IF ( NOT BUILD_LIBSBML )
    # Find on platform
    message(STATUS "Find libSBML on the platform.")
	FIND_PACKAGE(libSBML)
ENDIF()

# Build locally
IF ( NOT LIBSBML_FOUND OR BUILD_LIBSBML )
    message(STATUS "Building libSBML internally ...")
    SET(BUILD_LIBSBML TRUE CACHE BOOL "Build libSBML from source")
    SET(LIBSBML_STATIC TRUE CACHE BOOL "Link the libSBML Library statically." FORCE)
    add_subdirectory(${CMAKE_SOURCE_DIR}/3rdparty/libSBML ${CMAKE_BINARY_DIR}/3rdparty/libSBML)
ELSE()
    SET(BUILD_LIBSBML FALSE CACHE BOOL "Build libSBML from source")
ENDIF()

IF(LIBSBML_FOUND AND MORPHEUS_SBML)
	SET(SOURCES ${SOURCES} sbml_converter.cpp)
ENDIF()

##################################
##  Other dependencies          ##
##################################

FIND_PACKAGE(ZLIB REQUIRED)

#add_subdirectory(icons)

#OPTION(MORPHEUS_REMOTE "Enable support for remote computing resources. Depends on libSSH." OFF)


#IF (MORPHEUS_REMOTE)
#	FIND_PACKAGE(SSH)
#	IF(NOT LIBSSH_FOUND)
#		SET(MORPHEUS_REMOTE OFF FORCE)
#		MESSAGE(STATUS "LIBSSH not found. Remote computing support disabled.")
#	ENDIF()
#ENDIF()


### set the source, headers and resources
SET( SOURCES ${SOURCES}
    about_model.cpp
    abstractprocess.cpp 
    addattrdialog.cpp 
    attrcontroller.cpp 
    config.cpp 
    docu_dock.cpp
    domnodeviewer.cpp
    domnodeeditor.cpp
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
    model_index_mime_data.cpp
    morpheusML/model_attribute.cpp 
    morpheusML/model_node.cpp 
    morpheusML/morpheus_model.cpp
    morpheusML/morpheus_xml.cpp
    morpheusML/nodeadapter.cpp
    morpheusML/paramsweepmodel.cpp
    moviedialog.cpp
    network_access.cpp
    parametersweeper.cpp
    remoteprocess.cpp 
    settingsdialog.cpp 
    sshproxy.cpp 
    synchronizer.cpp 
    uri_handler.cpp
    xmlpreview.cpp 
    xsd.cpp
    widgets/checkboxlist.cpp
    widgets/equationhighlighter.cpp 
    widgets/mathtextedit.cpp 
    widgets/webviewer.cpp
    widgets/xmlhighlighter.cpp 
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

configure_file(generated_files.qrc.in generated_files.qrc @ONLY)

# compile the resources
SET( RESOURCES icons.qrc ${CMAKE_CURRENT_BINARY_DIR}/generated_files.qrc)
IF(MORPHEUS_GRAPHVIZ STREQUAL "WebRenderer")
	LIST(APPEND RESOURCES graphviz_renderer.qrc)
	set_property(SOURCE about_model.cpp APPEND PROPERTY COMPILE_DEFINITIONS GRAPHVIZ_WEB_RENDERER=1 )
ENDIF()


IF( WIN32 )
	SET( ICON_RESOURCE icons/win/morpheus.rc )
	ADD_CUSTOM_COMMAND( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/icon.o
		COMMAND windres.exe ARGS -i ${CMAKE_CURRENT_SOURCE_DIR}/${ICON_RESOURCE} -o ${CMAKE_CURRENT_BINARY_DIR}/icon.o
	DEPENDS ${ICON_RESOURCE} )
	LIST(APPEND SOURCES ${CMAKE_CURRENT_BINARY_DIR}/icon.o )
ENDIF()


### run moc and rcc over the headers and resource files
SET(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTORCC ON)
SET(CMAKE_AUTORCC_OPTIONS "--compress" "8")
SET(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(appdoc)
ADD_EXECUTABLE( MorpheusGUI WIN32 ${SOURCES} ${RESOURCES} )
set_target_properties(MorpheusGUI PROPERTIES RUNTIME_OUTPUT_NAME ${GUI_EXEC_NAME})

ADD_DEPENDENCIES(MorpheusGUI xmlSchema AppDoc)
# SET_PROPERTY(SOURCE ${CMAKE_CURRENT_BINARY_DIR}/ext_files.qrc PROPERTIES DEPENDS AppDoc)
	
get_target_property( res AppDoc RESOURCE)
set_property( TARGET MorpheusGUI APPEND PROPERTY RESOURCE ${res})

# IF (MORPHEUS_STATIC_BUILD)
# 	SET_TARGET_PROPERTIES(${GUI_EXEC_NAME} PROPERTIES LINK_SEARCH_START_STATIC 1)
# 	SET_TARGET_PROPERTIES(${GUI_EXEC_NAME} PROPERTIES LINK_SEARCH_END_STATIC 1)
# ENDIF()

INCLUDE_DIRECTORIES( ${ZLIB_INCLUDE_DIR}) 

### create executable and link to Qt libraries and external libraries
TARGET_LINK_LIBRARIES(MorpheusGUI Qt5::Widgets Qt5::Xml Qt5::Sql Qt5::Svg Qt5::Help Qt5::Network qtsingleapp ${ZLIB_LIBRARY} )

IF( WIN32 )
	IF( CMAKE_COMPILER_IS_GNUCXX )
		target_compile_options(MorpheusGUI PRIVATE $<IF:$<CONFIG:Release>,-mwindows,-mconsole>)
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
	#INCLUDE_DIRECTORIES(${LIBSBML_INCLUDE_DIR})
	target_compile_definitions(MorpheusGUI PUBLIC "HAVE_LIBSBML")
	TARGET_LINK_LIBRARIES_PATCHED(MorpheusGUI SBML::SBML)
	#IF(BUILD_LIBSBML)
        #ADD_DEPENDENCIES(MorpheusGUI libSBML)
	#ENDIF()
	
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
	INSTALL( TARGETS MorpheusGUI 
	         RUNTIME DESTINATION bin
	         RESOURCE DESTINATION share/morpheus
		 COMPONENT Morpheus_GUI
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
		 COMPONENT Morpheus_GUI
	)
	INSTALL(
		CODE "
		execute_process(COMMAND ${XDG-DESKTOP-MENU_EXECUTABLE} install --novendor ${CMAKE_INSTALL_PREFIX}/share/applications/morpheus.desktop) "
		COMPONENT Morpheus_GUI
	)
	
ELSEIF("${MORPHEUS_OS}" STREQUAL "APPLE")

	IF(MORPHEUS_RELEASE_BUNDLE)
		## We create manually installation suitible for Mac deployment through homebrew deployment. 
		## the GUI app is installed as Morpheus.app bundle, but the executable
		## morpheus simulator is installed within the bundle,
		## Thus, morpheus simulator can run from the command line, and the gui app can be integrated via "ln -s /usr/local/bin/MorpheusGUI.app /Applications/"
		## Also we provide a self-contained bundle when installing the DragNDrop_EXTRA component
	
		
		## Info.plist
		SET(MACOSX_GUI_COPYRIGHT "LICENSE.rtf")
		get_target_property(GUI_EXEC MorpheusGUI RUNTIME_OUTPUT_NAME)
		SET(MACOSX_BUNDLE_EXECUTABLE ${GUI_EXEC} )
		SET(MACOSX_BUNDLE_VERSION "${PROJECT_VERSION}")
		SET(MACOSX_BUNDLE_SHORT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
		configure_file(${PROJECT_SOURCE_DIR}/packaging/mac/Info.plist.in Info.plist @ONLY)
		
		#add_custom_target(MorpheusGUI_Bundle)
		#add_dependencies(MorpheusGUI_Bundle MorpheusGUI)
		#set_target_properties(MorpheusGUI_Bundle PROPERTIES
		#	RUNTIME_OUTPUT_NAME  $<MorpheusGui:RUNTIME_OUTPUT_NAME> )
		
		SET(BUNDLE_RESOURCES icons/mac/Morpheus.icns ${PROJECT_SOURCE_DIR}/${MACOSX_GUI_COPYRIGHT} )

		set(BUNDLE_PREFIX "${MORPHEUS_BUNDLE_DIR}/Contents")
		set(INSTALL_RUNTIME_DIR "${BUNDLE_PREFIX}/MacOS")
		set(INSTALL_RESOURCE_DIR "${BUNDLE_PREFIX}/Resources")

		set_property( TARGET MorpheusGUI 
			APPEND PROPERTY 
			RESOURCE ${BUNDLE_RESOURCES})

		INSTALL( FILES ${CMAKE_CURRENT_BINARY_DIR}/Info.plist 
			 DESTINATION ${MORPHEUS_BUNDLE_DIR}/Contents/ 
			 COMPONENT Morpheus_GUI
		)
		INSTALL( TARGETS MorpheusGUI 
		         RUNTIME DESTINATION ${INSTALL_RUNTIME_DIR}
		         RESOURCE DESTINATION ${INSTALL_RESOURCE_DIR}
			 COMPONENT Morpheus_GUI
		)
		
		## Add all dependencies in the Morpheus_GUI_SC COMPONENT
		# based on code from CMake's QtDialog/CMakeLists.txt
		macro(install_qt5_plugin _qt_plugin_name _qt_plugins_var _prefix)
		    get_target_property(_qt_plugin_path "${_qt_plugin_name}" LOCATION)
		    if(EXISTS "${_qt_plugin_path}")
			get_filename_component(_qt_plugin_file "${_qt_plugin_path}" NAME)
			get_filename_component(_qt_plugin_type "${_qt_plugin_path}" PATH)
			get_filename_component(_qt_plugin_type "${_qt_plugin_type}" NAME)
			set(_qt_plugin_dest "${_prefix}/PlugIns/${_qt_plugin_type}")
			install(FILES "${_qt_plugin_path}"
				DESTINATION "${_qt_plugin_dest}" 
				COMPONENT Morpheus_GUI_SC
				EXCLUDE_FROM_ALL)
			set(${_qt_plugins_var}
			    "${${_qt_plugins_var}};\$ENV{DEST_DIR}\${CMAKE_INSTALL_PREFIX}/${_qt_plugin_dest}/${_qt_plugin_file}")
		    else()
			message(FATAL_ERROR "QT plugin ${_qt_plugin_name} not found")
		    endif()
		endmacro()

		install_qt5_plugin("Qt5::QSQLiteDriverPlugin" QT_PLUGINS ${BUNDLE_PREFIX})
		install_qt5_plugin("Qt5::QSvgIconPlugin" QT_PLUGINS ${BUNDLE_PREFIX})
		install_qt5_plugin("Qt5::QSvgPlugin" QT_PLUGINS ${BUNDLE_PREFIX})
		install_qt5_plugin("Qt5::QSvgPlugin" QT_PLUGINS ${BUNDLE_PREFIX})
		install_qt5_plugin("Qt5::QTiffPlugin" QT_PLUGINS ${BUNDLE_PREFIX})
		if (APPLE)
			install_qt5_plugin("Qt5::QCocoaIntegrationPlugin" QT_PLUGINS ${BUNDLE_PREFIX})
			install_qt5_plugin("Qt5::QMacStylePlugin" QT_PLUGINS ${BUNDLE_PREFIX})
		elseif (UNIX)
			install_qt5_plugin("Qt5::QXdgDesktopPortalThemePlugin" QT_PLUGINS ${BUNDLE_PREFIX})
		else()
		endif()
		file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/qt.conf"
    			"[Paths]\nPlugins = ${_qt_plugin_dir}\n")
		install(FILES "${CMAKE_CURRENT_BINARY_DIR}/qt.conf"
    			DESTINATION "${INSTALL_RESOURCE_DIR}" 
			COMPONENT Morpheus_GUI_SC
			EXCLUDE_FROM_ALL)

		IF ( USE_QWebEngine )
			#QtWebEngine has addiditional requirements
			# dump_target_properties(Qt5::WebEngineCore)
			get_target_property(qt_webengine_dirs Qt5::WebEngineCore INTERFACE_INCLUDE_DIRECTORIES )
			macro(install_qwebengine_res _type _rel_path _search_dirs )
				set(res)
				foreach(dir ${${_search_dirs}})
					if(EXISTS ${dir}/${_rel_path})
						set(res ${dir}/${_rel_path})
					endif()
				endforeach()
				if(res)
					if(${_type} STREQUAL "PROGRAM")
						INSTALL(PROGRAMS ${res} 
							DESTINATION ${INSTALL_RUNTIME_DIR} 
							COMPONENT Morpheus_GUI_SC
							EXCLUDE_FROM_ALL)
					elseif(${_type} STREQUAL "RESOURCE")
						INSTALL(FILES ${res} 
							DESTINATION ${INSTALL_RESOURCE_DIR} 
							COMPONENT Morpheus_GUI_SC
							EXCLUDE_FROM_ALL)
					else()
						MESSAGE(FATAL_ERROR "Invalid resource type ${_type}")
					endif()
				else()
					message(SEND_ERROR "Unable to locate QWebengine resource ${_rel_path}" )
				endif()
			endmacro()

			install_qwebengine_res(PROGRAM Helpers/QtWebEngineProcess.app/Contents/MacOS/QtWebEngineProcess qt_webengine_dirs)
			install_qwebengine_res(RESOURCE Resources/qtwebengine_resources.pak qt_webengine_dirs)
			install_qwebengine_res(RESOURCE Resources/qtwebengine_resources_100p.pak qt_webengine_dirs)
			install_qwebengine_res(RESOURCE Resources/qtwebengine_resources_200p.pak qt_webengine_dirs)
		endif()

		set(APPS "\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${MORPHEUS_BUNDLE_DIR}")
		# Directories to look for dependencies
		set(DIRS "${CMAKE_BINARY_DIR}")

		# Path used for searching by FIND_XXX(), with appropriate suffixes added
		if(CMAKE_PREFIX_PATH)
    			foreach(dir ${CMAKE_PREFIX_PATH})
        			list(APPEND DIRS "${dir}/bin" "${dir}/lib")
			endforeach()
		endif()

		# Append Qt's lib folder which is two levels above Qt5Widgets_DIR
		list(APPEND BUNDLE_DIRS "${Qt5Widgets_DIR}/../..")

		MESSAGE(STATUS "GUI exec is ${GUI_EXEC}")
		message(STATUS "APPS: ${APPS}")
		message(STATUS "QT_PLUGINS: ${QT_PLUGINS}")
		message(STATUS "DIRS: ${DIRS}")
		# include(InstallRequiredSystemLibraries)
		INSTALL(CODE "
			INCLUDE(BundleUtilities)
			SET(BU_CHMOD_BUNDLE_ITEMS \"TRUE\")
			fixup_bundle(\"${APPS}\" \"${QT_PLUGINS}\" \"${DIRS}\") "
			COMPONENT Morpheus_GUI_SC
			EXCLUDE_FROM_ALL
		)
	
	ELSE () 
		INSTALL( TARGETS MorpheusGUI 
		         RUNTIME DESTINATION bin
		         RESOURCE DESTINATION share/morpheus
			 COMPONENT Morpheus_GUI
		)
	ENDIF()

ELSEIF("${MORPHEUS_OS}" STREQUAL "WIN32")
	set(BUNDLE_PREFIX ".")
	set(INSTALL_RUNTIME_DIR "${BUNDLE_PREFIX}")
	set(INSTALL_RESOURCE_DIR "${BUNDLE_PREFIX}")

	# set_property( TARGET MorpheusGUI 
		# APPEND PROPERTY 
		# RESOURCE ${BUNDLE_RESOURCES})

	INSTALL( TARGETS MorpheusGUI
			 COMPONENT Morpheus_GUI
			 RUNTIME DESTINATION "${INSTALL_RUNTIME_DIR}"
			 RESOURCE DESTINATION "${INSTALL_RESOURCE_DIR}"
			 )
		
	## Add all dependencies in the Morpheus_GUI_SC COMPONENT
	# based on code from CMake's QtDialog/CMakeLists.txt
	macro(install_qt5_plugin _qt_plugin_name _qt_plugins_var _prefix)
		get_target_property(_qt_plugin_path "${_qt_plugin_name}" LOCATION)
		if(EXISTS "${_qt_plugin_path}")
		get_filename_component(_qt_plugin_file "${_qt_plugin_path}" NAME)
		get_filename_component(_qt_plugin_type "${_qt_plugin_path}" PATH)
		get_filename_component(_qt_plugin_type "${_qt_plugin_type}" NAME)
		set(_qt_plugin_dest "${_prefix}/PlugIns/${_qt_plugin_type}")
		install(FILES "${_qt_plugin_path}"
			DESTINATION "${_qt_plugin_dest}" 
			COMPONENT Morpheus_GUI
			EXCLUDE_FROM_ALL)
		set(${_qt_plugins_var}
			"${${_qt_plugins_var}};\$ENV{DEST_DIR}\${CMAKE_INSTALL_PREFIX}/${_qt_plugin_dest}/${_qt_plugin_file}")
		else()
		message(FATAL_ERROR "QT plugin ${_qt_plugin_name} not found")
		endif()
	endmacro()

	install_qt5_plugin("Qt5::QSQLiteDriverPlugin" QT_PLUGINS ${BUNDLE_PREFIX})
	install_qt5_plugin("Qt5::QSvgIconPlugin" QT_PLUGINS ${BUNDLE_PREFIX})
	install_qt5_plugin("Qt5::QSvgPlugin" QT_PLUGINS ${BUNDLE_PREFIX})
	install_qt5_plugin("Qt5::QTiffPlugin" QT_PLUGINS ${BUNDLE_PREFIX})
	install_qt5_plugin("Qt5::QGenericEnginePlugin" QT_PLUGINS ${BUNDLE_PREFIX})
	
	if (APPLE)
		install_qt5_plugin("Qt5::QCocoaIntegrationPlugin" QT_PLUGINS ${BUNDLE_PREFIX})
		install_qt5_plugin("Qt5::QMacStylePlugin" QT_PLUGINS ${BUNDLE_PREFIX})
	elseif(WIN32)
		install_qt5_plugin("Qt5::QWindowsIntegrationPlugin" QT_PLUGINS ${BUNDLE_PREFIX})
		install_qt5_plugin("Qt5::QWindowsVistaStylePlugin" QT_PLUGINS ${BUNDLE_PREFIX})
		find_file(LIBSSL_DLL NAMES libssl-1_1.dll libssl-1_1-x64.dll DOC "Dynamic SSL library" REQUIRED)
		find_file(LIBCRYPTO_DLL NAMES libcrypto-1_1.dll libcrypto-1_1-x64.dll DOC "Dynamic Crypto library" REQUIRED)
		install(FILES ${LIBSSL_DLL} ${LIBCRYPTO_DLL}
			DESTINATION .
			COMPONENT Morpheus_GUI
			EXCLUDE_FROM_ALL)
		# get_filename_component(LIB_SSL_DLL_FILENAME ${LIBSSL_DLL} NAME)
		# list(APPEND QT_PLUgins "\$ENV{DEST_DIR}\${CMAKE_INSTALL_PREFIX}/${LIBSSL_DLL_FILENAME}" )
	elseif(UNIX)
		install_qt5_plugin("Qt5::QXdgDesktopPortalThemePlugin" QT_PLUGINS ${BUNDLE_PREFIX})
	else()
	endif()
	
	file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/qt.conf"
			"[Paths]\nPlugins = PlugIns\n")
	install(FILES "${CMAKE_CURRENT_BINARY_DIR}/qt.conf"
			DESTINATION "${INSTALL_RESOURCE_DIR}" 
		COMPONENT Morpheus_GUI
		EXCLUDE_FROM_ALL)

		
	IF ( USE_QWebEngine )
		#QtWebEngine has addiditional requirements
		# dump_target_properties(Qt5::WebEngineCore)
		get_target_property(qt_webengine_dirs Qt5::WebEngineCore INTERFACE_INCLUDE_DIRECTORIES )
		macro(install_qwebengine_res _type _rel_path _search_dirs )
			set(res)
			foreach(dir ${${_search_dirs}})
				if(EXISTS ${dir}/${_rel_path})
					set(res ${dir}/${_rel_path})
				endif()
			endforeach()
			if(res)
				if(${_type} STREQUAL "PROGRAM")
					INSTALL(PROGRAMS ${res} 
						DESTINATION ${INSTALL_RUNTIME_DIR} 
						COMPONENT Morpheus_GUI
						EXCLUDE_FROM_ALL)
				elseif(${_type} STREQUAL "RESOURCE")
					INSTALL(FILES ${res} 
						DESTINATION ${INSTALL_RESOURCE_DIR} 
						COMPONENT Morpheus_GUI
						EXCLUDE_FROM_ALL)
				else()
					MESSAGE(FATAL_ERROR "Invalid resource type ${_type}")
				endif()
			else()
				message(SEND_ERROR "Unable to locate QWebengine resource ${_rel_path}" )
			endif()
		endmacro()

		install_qwebengine_res(PROGRAM Helpers/QtWebEngineProcess.app/Contents/MacOS/QtWebEngineProcess qt_webengine_dirs)
		install_qwebengine_res(RESOURCE Resources/qtwebengine_resources.pak qt_webengine_dirs)
		install_qwebengine_res(RESOURCE Resources/qtwebengine_resources_100p.pak qt_webengine_dirs)
		install_qwebengine_res(RESOURCE Resources/qtwebengine_resources_200p.pak qt_webengine_dirs)
	endif()

	set(APPS "\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${MORPHEUS_BUNDLE_PREFIX}")
	# Directories to look for dependencies
	set(DIRS "${CMAKE_BINARY_DIR}")

	# Path used for searching by FIND_XXX(), with appropriate suffixes added
	LIST(APPEND CMAKE_PREFIX_PATH "C:/msys64/mingw64" "${CMAKE_SYSROOT_LINK}")
	if(CMAKE_PREFIX_PATH)
			foreach(dir ${CMAKE_PREFIX_PATH})
				list(APPEND DIRS "${dir}/bin" "${dir}/lib")
		endforeach()
	endif()
	# Append Qt's lib folder which is two levels above Qt5Widgets_DIR
	#list(APPEND BUNDLE_DIRS "${Qt5Widgets_DIR}/../..")
	
	INSTALL(CODE "
		SET(EXECUTABLES \"${APPS}/morpheus-gui.exe\")
		#MESSAGE(\"CPACK: - Searching binaries in ${APPS}/..\")
		#file(GLOB_RECURSE EXECUTABLES \"\${APPS}/*.exe\")
		#file(GLOB_RECURSE DLLS \"${APPS}/*.dll\")
		message(\"CPACK:   - APPS: \${EXECUTABLES}\")
		message(\"CPACK:   - QT_PLUGINS: ${QT_PLUGINS}\")
		message(\"CPACK:   - DIRS: ${DIRS}\")
		LIST(APPEND EXECUTABLES \"${QT_PLUGINS}\")
		file(GET_RUNTIME_DEPENDENCIES
			RESOLVED_DEPENDENCIES_VAR deps_resolved
			UNRESOLVED_DEPENDENCIES_VAR deps_unresolved
			EXECUTABLES \${EXECUTABLES}
			DIRECTORIES ${DIRS})
		foreach(dep \${deps_resolved})
			if( dep MATCHES \"C:[/\\\\]Windows.*\")
				# do not install system libraries
				# MESSAGE(\"CPACK:  - not adding \${dep}\")
			else()
				MESSAGE(\"CPACK:  - adding \${dep}\")
				file(INSTALL \"\${dep}\"
				DESTINATION \"${APPS}/${INSTALL_RUNTIME_DIR}\")
			endif()
		endforeach()
		# MESSAGE(\"CPACK:  - Resolved dependencies \${deps_resolved}\")
		# MESSAGE(\"CPACK:  - Unresolved dependencies \${deps_unresolved}\")
		"
		COMPONENT Morpheus_GUI
		EXCLUDE_FROM_ALL
	)
	
ENDIF()

SET(LIBSBML_FOUND ${LIBSBML_FOUND} PARENT_SCOPE)
