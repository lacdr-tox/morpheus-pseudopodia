MESSAGE(STATUS "Configuring environment for packaging Morpheus into ${CPACK_GENERATOR}")

## Here we setup specific parameters per generator -- No build here 

LIST(REMOVE_ITEM CPACK_COMPONENTS_ALL Morpheus_SC Morpheus_GUI_SC)
MESSAGE(STATUS "All DEFAULT COMPONENTS ${CPACK_COMPONENTS_ALL}")
set(CPACK_COMPONENTS_GROUPING ALL_COMPONENTS_IN_ONE)

# Append the tag at packaging time
MESSAGE(STATUS "CPACK: Tag ${CPACK_PACKAGE_NAME_TAG}")
IF (DEFINED CPACK_PACKAGE_NAME_TAG)
	STRING(APPEND CPACK_PACKAGE_FILE_NAME "_${CPACK_PACKAGE_NAME_TAG}")
ENDIF()

IF(CPACK_GENERATOR STREQUAL "DEB")
	LIST(APPEND CPACK_COMPONENTS_ALL Morpheus_DEB_EXTRAS)

  ## Include System Architecture into package name
#   FIND_PROGRAM(DPKG_PROGRAM dpkg DOC "dpkg package manager of Debian-based systems")
#   IF(DPKG_PROGRAM) 
#     EXECUTE_PROCESS( 
#       COMMAND ${DPKG_PROGRAM} --print-architecture 
#       OUTPUT_VARIABLE CPACK_DEBIAN_PACKAGE_ARCHITECTURE 
#       OUTPUT_STRIP_TRAILING_WHITESPACE 
#     )
#     STRING(APPEND CPACK_PACKAGE_FILE_NAME "_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")
#   ENDIF()

	# Append the tag at packaging time
	IF (DEFINED CPACK_PACKAGE_NAME_TAG)
		STRING(APPEND CPACK_DEBIAN_FILE_NAME "~${CPACK_PACKAGE_NAME_TAG}")
	ENDIF()

ELSEIF(CPACK_GENERATOR STREQUAL "DragNDrop")
	LIST(APPEND CPACK_COMPONENTS_ALL Morpheus_SC Morpheus_GUI_SC)
	
ELSEIF(CPACK_GENERATOR STREQUAL "NSIS")
	#LIST(APPEND CPACK_COMPONENTS_ALL Morpheus_SC Morpheus_GUI_SC)
	set(CPACK_COMPONENTS_GROUPING ALL_COMPONENTS_IN_ONE)
	SET(CPACK_PACKAGE_VENDOR "TU-Dresden")
	

ELSEIF(CPACK_GENERATOR STREQUAL "WIX")  
	SET(CPACK_WIX_PRODUCT_GUID "D80BAC6B-539F-4384-AD1B-FCDD8091DE76")
	SET(CPACK_WIX_UPGRADE_GUID ${CPACK_WIX_PRODUCT_GUID})
	SET(CPACK_WIX_PRODUCT_ICON "${CMAKE_CURRENT_SOURCE_DIR}/gui/icons/win/morpheus.ico")
	SET(CPACK_WIX_PROPERTY_ARPURLINFOABOUT ${CPACK_PACKAGE_HOMEPAGE} )
	
	LIST(APPEND CPACK_COMPONENTS_ALL Morpheus_SC Morpheus_GUI_SC)
ENDIF()
