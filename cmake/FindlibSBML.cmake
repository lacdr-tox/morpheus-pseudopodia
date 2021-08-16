# Locate libsbml
# This module defines:
# LIBSBML_INCLUDE_DIR, where to find the headers
#
# LIBSBML_LIBRARY, LIBSBML_LIBRARY_DEBUG
# LIBSBML_FOUND
#
# $LIBSBML_DIR is an environment variable that would
# correspond to the ./configure --prefix=$LIBSBML_DIR
#
# Created by Robert Osfield.
# Modified by Ralph Gauges
# Copied from COPASI, thx to the authors

set(LIBSBML_FOUND FALSE)
IF (LIBSBML_STATIC)
	SET(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_STATIC_LIBRARY_SUFFIX} )
	SET(LIBSBML_LIB_NAMES sbml-static libsbml-static sbml libsbml)
ELSEIF(CMAKE_FIND_LIBRARY_SUFFIXES STREQUAL ${CMAKE_STATIC_LIBRARY_SUFFIX})
	SET(LIBSBML_STATIC TRUE)
	SET(LIBSBML_LIB_NAMES sbml-static libsbml-static sbml libsbml)
ELSE()
	SET(LIBSBML_STATIC FALSE)
	SET(LIBSBML_LIB_NAMES sbml libsbml)
ENDIF()

find_path(LIBSBML_INCLUDE_DIR sbml/SBase.h
    PATHS $ENV{LIBSBML_DIR}/include
          $ENV{LIBSBML_DIR}
          ~/Library/Frameworks
          /Library/Frameworks
          /usr/local/include
          /usr/include/
          /sw/include # Fink
          /opt/local/include # MacPorts
          /opt/csw/include # Blastwave
          /opt/include
          /usr/freeware/include
)

find_library(LIBSBML_LIBRARY
    NAMES ${LIBSBML_LIB_NAMES}
    PATHS $ENV{LIBSBML_DIR}/lib
          $ENV{LIBSBML_DIR}
          ~/Library/Frameworks
          /Library/Frameworks
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
          /sw/lib # Fink
          /opt/local/lib # MacPorts
          /opt/csw/lib # Blastwave
          /opt/lib
          /usr/freeware/lib64
)

message(STATUS "SBML found at ${LIBSBML_LIBRARY} ${LIBSBML_INCLUDE_DIR}")
# handle the QUIETLY and REQUIRED arguments and set LIBSBML_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LIBSBML DEFAULT_MSG LIBSBML_LIBRARY LIBSBML_INCLUDE_DIR)

IF(LIBSBML_LIBRARY AND LIBSBML_INCLUDE_DIR)
	IF (LIBSBML_STATIC)
		ADD_LIBRARY(SBML::SBML STATIC IMPORTED)
		target_compile_definitions(SBML::SBML INTERFACE "LIBSBML_STATIC")
		# Prefind all required libraries
		find_package(LIBXML2_LIBRARY xml2)
		find_library(ZLIB_LIBRARY z)
		find_library(BZIP2_LIBRARY bz2)
		target_link_libraries(SBML::SBML INTERFACE ${LIBXML2_LIBRARY} ${ZLIB_LIBRARY} ${BZIP2_LIBRARY})
	ELSE()
		ADD_LIBRARY(SBML::SBML SHARED IMPORTED)
	ENDIF()
	set_target_properties(SBML::SBML PROPERTIES
		IMPORTED_LOCATION ${LIBSBML_LIBRARY}
		INTERFACE_INCLUDE_DIRECTORIES ${LIBSBML_INCLUDE_DIR}
		)
ENDIF()

