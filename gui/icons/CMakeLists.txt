find_program(INKSCAPE NAMES "inkscape")
find_program(CONVERT NAMES "convert")

SET(ICON_SOURCE "icon_new.svg" )
SET(ICON_SIZES 16 22 36 48 64 128 256 512 )

IF(INKSCAPE AND CONVERT)
	message(STATUS "Found inkscape and convert, added update_icons target" )
	FOREACH( ICON_SIZE xsd_file ${ICON_SIZES})
		SET(ICON_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/${ICON_SIZE}x${ICON_SIZE} )
		FILE( MAKE_DIRECTORY ${ICON_FOLDER})
		# the plain version
		SET( ICON  ${ICON_FOLDER}/icon.png )
		SET( ICONS ${ICONS} ${ICON})
		ADD_CUSTOM_COMMAND(
			OUTPUT ${ICON}
			COMMAND ${INKSCAPE} -z --export-id="plain_icon" --export-png=${ICON} -w ${ICON_SIZE} -h ${ICON_SIZE} ${ICON_SOURCE}
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
			DEPENDS ${ICON_SOURCE}
			COMMENT "Generating icon ${ICON}"
		)
		# the running version
		SET( ICON  ${ICON_FOLDER}/icon_running.png )
		SET( ICONS ${ICONS} ${ICON})
		ADD_CUSTOM_COMMAND(
			OUTPUT ${ICON}
			COMMAND ${INKSCAPE} -z --export-id="running_icon" --export-png=${ICON} -w ${ICON_SIZE} -h ${ICON_SIZE} ${ICON_SOURCE}
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
			DEPENDS ${ICON_SOURCE}
			COMMENT "Generating icon ${ICON}"
		)
		# the sweeping version
		SET( ICON  ${ICON_FOLDER}/icon_sweep.png )
		SET( ICONS ${ICONS} ${ICON})
		ADD_CUSTOM_COMMAND(
			OUTPUT ${ICON}
			COMMAND ${INKSCAPE} -z --export-id="sweep_icon" --export-png=${ICON} -w ${ICON_SIZE} -h ${ICON_SIZE} ${ICON_SOURCE}
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
			DEPENDS ${ICON_SOURCE}
			COMMENT "Generating icon ${ICON}"
		)
		
	ENDFOREACH()

	ADD_CUSTOM_TARGET("update_icons" DEPENDS ${ICONS})
	
ENDIF()
