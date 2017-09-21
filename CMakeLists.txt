PROJECT(morpheus)
cmake_minimum_required(VERSION 2.8.0)

IF(CMAKE_VERSION VERSION_GREATER "3.1" OR CMAKE_VERSION VERSION_EQUAL "3.1")
	CMAKE_POLICY(SET CMP0054 NEW)
ENDIF()
 
SET(VERSION_MAJOR "1")
SET(VERSION_MINOR "9")
SET(VERSION_PATCH "3")
SET(MORPHEUS_VERSION_STRING "\"${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}\"")

IF(CMAKE_VERSION VERSION_LESS "2.8.10")
	EXECUTE_PROCESS( COMMAND date +%y%m%d
		OUTPUT_VARIABLE MORPHEUS_REVISION
	        OUTPUT_STRIP_TRAILING_WHITESPACE )	
	EXECUTE_PROCESS( COMMAND date +\"%d.%m.%Y\"
		OUTPUT_VARIABLE MORPHEUS_REVISION_STRING
	        OUTPUT_STRIP_TRAILING_WHITESPACE )	
ELSE()
	STRING(TIMESTAMP MORPHEUS_REVISION "%y%m%d" )
	STRING(TIMESTAMP MORPHEUS_REVISION_STRING "\"%d.%m.%Y\"")
ENDIF()

MESSAGE(STATUS "MORPHEUS VERSION IS ${MORPHEUS_VERSION_STRING}, REVISION ${MORPHEUS_REVISION_STRING}" )

CONFIGURE_FILE("version.h.in" ${CMAKE_CURRENT_BINARY_DIR}/version.h @ONLY)
INCLUDE_DIRECTORIES( ${PROJECT_BINARY_DIR})

IF (CMAKE_VERSION VERSION_LESS "2.8.8")
	SET(HAVE_OBJECT_LINKAGE FALSE)
ELSE()
	SET(HAVE_OBJECT_LINKAGE TRUE)
ENDIF()
MESSAGE(STATUS "HAVE_OBJECT_LINKAGE " ${HAVE_OBJECT_LINKAGE})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

### PLATFORM DEPENDEND INSTALL DIRs of TARGETS
IF( UNIX AND NOT APPLE )
	SET(MORPHEUS_OS "UNIX")
	MESSAGE(STATUS "Compiling for platform *NIX.")
ELSEIF(APPLE)
	SET(MORPHEUS_OS "APPLE")
	MESSAGE(STATUS "Compiling for platform OSX.")
ELSEIF(WIN32)
	SET(MORPHEUS_OS "WIN32")
	SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -DWIN32")
	MESSAGE(STATUS "Compiling for platform WIN32.")
ELSE()
	SET(MORPHEUS_OS "UNKNOWN")
ENDIF()

IF( CMAKE_COMPILER_IS_GNUCXX )
	SET(STDCxx11FLAG "-std=gnu++11")
ELSE(CMAKE_COMPILER_IS_GNUCXX)
	SET(STDCxx11FLAG "-std=c++11")
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${STDCxx11FLAG} -Wno-deprecated-declarations -Wno-ignored-attributes")


## Switch to Release Build Mode
OPTION(MORPHEUS_RELEASE_BUNDLE "Build the release bundle of morpheus only." OFF)
# IF (MORPHEUS_RELEASE_BUNDLE) 
# 	SET( CMAKE_BUILD_TYPE "RELEASE" )
# ENDIF()

OPTION(MORPHEUS_STATIC_BUILD "Create a statically linked binary." OFF)
IF (MORPHEUS_STATIC_BUILD)
 SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libstdc++")
ENDIF()

# Configuration file for the uninstall target
CONFIGURE_FILE( "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
	IMMEDIATE @ONLY
)

ADD_CUSTOM_TARGET(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake )

IF(NOT CMAKE_BUILD_TYPE)
	SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
		"Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel." FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

OPTION(MORPHEUS_GUI "Build the graphical user interface." ON)

ADD_SUBDIRECTORY( 3rdparty )
INCLUDE_DIRECTORIES( 3rdparty)

FIND_PACKAGE(TIFF REQUIRED)
ADD_SUBDIRECTORY( morpheus )



#IF(NOT ${MORPHEUS_OS} STREQUAL "WIN32")
# ADD_SUBDIRECTORY( Scripts )
#ENDIF()

IF(MORPHEUS_GUI)
	ADD_SUBDIRECTORY( Examples )
	ADD_SUBDIRECTORY( gui )
ENDIF()

add_custom_target( CompressedChangelog ALL
COMMAND gzip -c -n -9 -f  ${CMAKE_CURRENT_SOURCE_DIR}/CHANGELOG.md >  ${CMAKE_CURRENT_BINARY_DIR}/changelog.gz
COMMAND cmake -E copy ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.md ${CMAKE_CURRENT_BINARY_DIR}/copyright
SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/CHANGELOG.md ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.md )

INCLUDE(InstallRequiredSystemLibraries)


SET(M_MAINTAINER "Joern Starruss <joern.starruss@tu-dresden.de>")
SET(M_APP_NAME "Morpheus")
SET(M_EXECUTABLES ${SIMULATOR_EXEC_NAME} ${GUI_EXEC_NAME})

STRING(TOLOWER "${M_APP_NAME}" CPACK_PACKAGE_NAME )
# SET(CPACK_PACKAGE_NAME "${M_APP_NAME}")
SET(CPACK_PACKAGE_EXECUTABLES  ${GUI_EXEC_NAME}; "Morpheus - Modeling Morphogenesis")

SET(CPACK_PACKAGE_VERSION_MAJOR ${VERSION_MAJOR} )
SET(CPACK_PACKAGE_VERSION_MINOR ${VERSION_MINOR} )
SET(CPACK_PACKAGE_VERSION_PATCH ${VERSION_PATCH} )


SET(CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_NAME}")

INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/changelog.gz ${CMAKE_CURRENT_BINARY_DIR}/copyright
DESTINATION "share/doc/${CPACK_PACKAGE_NAME}/")

#-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}")
#SET(CPACK_PACKAGE_VENDOR "IMC, Center for High Performance Computing, TU Dresden")
SET(CPACK_PACKAGE_VENDOR "Jörn Starruß & Walter de Back (ZIH, TU-Dresden)")


SET(CPACK_PACKAGE_CONTACT ${M_MAINTAINER})

# SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README")

SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.md")


SET(CPACK_STRIP_FILES "${M_EXECUTABLES}")
#SET(CPACK_SOURCE_STRIP_FILES "")





IF( "${MORPHEUS_OS}" STREQUAL "UNIX" AND  NOT "${MORPHEUS_OS}" STREQUAL "APPLE" )
  
  SET(CPACK_DEBIAN_PACKAGE_DESCRIPTION "Modeling environment for Multi-Cellular
  Systems Biology
  Modeling and simulation environment for multi-cellular systems biology.
  Includes simulators for ODEs, PDEs and cellular Potts models. 
  Facilitates model integration into multiscale models.
  .
  - Modeling without programming.
  - Runtime parsing of math expressions.
  - Multiscale integration by symbolic linking.
  - Parameter exploration through (parallel) batch processing.
  - Multi-core concurrency support using openMP multithreading.
  - Support for use of remote high performance computing resources.")
  SET(CPACK_DEBIAN_PACKAGE_SECTION "science")
  SET(CPACK_DEBIAN_PACKAGE_MAINTAINTER ${M_MAINTAINTER})
  
 
  
  FIND_PROGRAM(DPKG_PROGRAM dpkg DOC "dpkg program of Debian-based systems")
  
  SET(DEBIAN_TIFF_PACKAGE "libtiff5")
  
  SET(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.3.6), libgomp1, libgcc1 (>= 1:4.1), libssh-4, ${DEBIAN_TIFF_PACKAGE}, libqtcore4, libqtgui4, libqt4-xml, libqt4-help, libqt4-svg, libqt4-sql-sqlite, libqtwebkit4, gnuplot-qt (>= 4.4) | gnuplot-x11 (>=4.4), graphviz")
  
  IF(LIBSBML_FOUND AND MORPHEUS_SBML)
    SET(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}, libsbml5")
  ENDIF()
  
  SET(CPACK_GENERATOR "DEB")

  SET(CPACK_DEBIAN_PACKAGE_HOMEPAGE "http://imc.zih.tu-dresden.de/wiki/morpheus")

  STRING(TOLOWER "${CPACK_PACKAGE_NAME}" CPACK_PACKAGE_NAME_LOWERCASE)
 
  IF(DPKG_PROGRAM)
    EXECUTE_PROCESS(
      COMMAND ${DPKG_PROGRAM} --print-architecture
      OUTPUT_VARIABLE CPACK_DEBIAN_PACKAGE_ARCHITECTURE
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    SET(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME_LOWERCASE}_${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}~b${MORPHEUS_REVISION}")   
  ELSE(DPKG_PROGRAM)
    SET(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME_LOWERCASE}_${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}_${CMAKE_SYSTEM_NAME}~b${MORPHEUS_REVISION}")
  ENDIF(DPKG_PROGRAM)
  

ELSEIF("${MORPHEUS_OS}" STREQUAL "APPLE")
 	SET( PROGNAME Morpheus )
 	MESSAGE("Getting morpheus simulator and XSD file ../../morpheus. Note: We assuma a proper folder structure (1) morpheus/build and (2) morpheus-gui/build where cpack is executed from folder (2).")
 	INSTALL( FILES ${CMAKE_SOURCE_DIR}/build/morpheus/core/morpheus
                         DESTINATION bin )
 	INSTALL( FILES ${CMAKE_SOURCE_DIR}/build/gui/morpheus-gui
                         DESTINATION bin )
 	INSTALL( FILES ${CMAKE_SOURCE_DIR}/build/xmlSchema/morpheus.xsd
                         DESTINATION share )
 	INSTALL( DIRECTORY ${CMAKE_SOURCE_DIR}/Examples
                         DESTINATION . )

	MESSAGE("Trying to pack bundle, source dir = " ${CMAKE_CURRENT_SOURCE_DIR} "; "  ${CMAKE_SOURCE_DIR} )			
	SET(CPACK_GENERATOR "Bundle")
	SET(CPACK_BUNDLE_NAME "Morpheus")
	SET(CPACK_PACKAGE_FILE_NAME "Morpheus_${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}") 
	SET(CPACK_DMG_VOLUME_NAME "Morpheus_${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")  # does not work?
	MESSAGE( "CPACK_DMG_VOLUME_NAME = ${CPACK_DMG_VOLUME_NAME}")
	SET(CPACK_BUNDLE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/gui/icons/mac/icon.icns")
	SET(CPACK_BUNDLE_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/gui/icons/mac/Info.plist")
	SET(CPACK_BUNDLE_STARTUP_COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/gui/icons/mac/startup.sh")
	#${CMAKE_CURRENT_SOURCE_DIR}/build/src/morpheus-gui")

ELSEIF("${MORPHEUS_OS}" STREQUAL "WIN32")
  SET(CPACK_GENERATOR "NSIS")
  SET(CPACK_MONOLITHIC_INSTALL 1)
  SET(CPACK_PACKAGE_EXECUTABLES "${GUI_EXEC_NAME};Morpheus")
  SET(CPACK_PACKAGE_NAME "${M_APP_NAME}")
  SET(CPACK_PACKAGE_FILE_NAME "morpheus-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}.${CMAKE_SYSTEM_NAME}.b${MORPHEUS_REVISION}")
  SET(CPACK_PACKAGE_INSTALL_REGISTRY_KEY morpheus)
  SET(CPACK_NSIS_DISPLAY_NAME "${M_APP_NAME} ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}")
  # : Modeling environment for Multiscale Morphogenesis")
  SET(CPACK_NSIS_CONTACT "${MAINTAINTER}")
  #SET(CPACK_PACKAGE_EXECUTABLES "${M_EXEC_NAME}" "${EWS_APP_NAME}")
  SET(CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\imc.zih.tu-dresden.de\\\\wiki\\\\morpheus")

  # create Start Menu links (application, uninstaller and website)
  SET(CPACK_NSIS_MENU_LINKS "${GUI_EXEC_NAME}" "${M_APP_NAME}")
  # install/uninstall Desktop item
  
  SET(CPACK_NSIS_EXTRA_INSTALL_COMMANDS
    "CreateShortCut \\\"$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\Morpheus Website.lnk\\\" \\\"http:\\\\\\\\imc.zih.tu-dresden.de\\\\wiki\\\\morpheus\\\" \n
     WriteRegStr HKCU \\\"Software\\\\Morpheus\\\\Morpheus\\\\local\\\" \\\"executable\\\" \\\"$INSTDIR\\\\morpheus.exe\\\"
  ")
  # CreateShortCut \\\"$DESKTOP\\\\${M_APP_NAME}.lnk\\\" \\\"$INSTDIR\\\\${GUI_EXEC_NAME}\\\" \n
  SET(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS 
    "Delete \\\"$DESKTOP\\\\${M_APP_NAME}.lnk\\\" \n
	 Delete \\\"$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\Morpheus Website.lnk\\\"
    ")

  # name as it appears in the Add/Remove Software panel
  SET(CPACK_NSIS_INSTALLED_ICON_NAME "${GUI_EXEC_NAME}")
  # installation directory of morpheus-GUI need NOT be set in the PATH
  SET(CPACK_NSIS_MODIFY_PATH "OFF")

  # icons ICO for the installer/uninstaller and BMP 'branding image' displayed within installer and uninstaller.
  SET(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/gui/icons/win/morpheus.ico")
  SET(CPACK_NSIS_MUI_UNIICON "${CMAKE_CURRENT_SOURCE_DIR}/gui/icons/win/morpheus.ico")
  #SET(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/gui/icons/win/morpheus.ico")
ENDIF()


INCLUDE(CPack)

# MESSAGE("Precompiler platform: __UNIX__ " ${__UNIX__} ", __APPLE__ " ${__APPLE__} ", __WIN32__ " ${__WIN32__})

