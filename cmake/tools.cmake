FUNCTION(list_prepend IN prefix OUT)
	SET(_list "")
	FOREACH(entry ${${IN}})
		LIST(APPEND _list "${prefix}${entry}")
	ENDFOREACH()
	SET(${OUT} ${_list} PARENT_SCOPE)
ENDFUNCTION()

## Add sources paths 
FUNCTION(target_sources_relpaths target iface sources)
	IF (CMAKE_VERSION VERSION_LESS 3.13)
		FOREACH(entry ${${sources}})
			target_sources(${target} ${iface} ${CMAKE_CURRENT_SOURCE_DIR}/${entry})
		ENDFOREACH()
	ELSE()
		target_sources(${target} ${iface} ${${sources}})
	ENDIF()
ENDFUNCTION()

## Add link libraries to a target, includes fixed handling for OBJECT_LIBRARY
## The fixed version provides proper treatment of object link libraries. 
## TODO Linker flags are not copied though.
FUNCTION(target_link_libraries_fixed target iface)
	set(libraries ${ARGN})
	if (CMAKE_VERSION VERSION_LESS 3.12)
		add_dependencies(${target} ${libraries})
		FOREACH(library ${libraries})
			IF(TARGET ${library})
				get_property(type TARGET ${library} PROPERTY TYPE)
				IF(${type} STREQUAL "OBJECT_LIBRARY")
					## Library is an object library target
					# Linking
					IF(${iface} STREQUAL "PRIVATE" OR ${iface} STREQUAL "PUBLIC")
						target_sources(${target} $<TARGET_OBJECTS:${library}> )
						target_link_libraries(${target} PRIVATE $<TARGET_PROPERTY:${library},INTERFACE_LINK_LIBRARIES> )
					ENDIF()
					# Publishing
					IF(${iface} STREQUAL "PUBLIC" OR ${iface} STREQUAL "INTERFACE")
						target_link_libraries(${target} INTERFACE ${library} )
					ENDIF()
				ELSE()
					target_link_libraries(${target} ${iface} ${library})
				ENDIF()
			ELSE()
				target_link_libraries(${target} ${iface} ${library})
			ENDIF()
			
		ENDFOREACH()
	ELSE()
		target_link_libraries(${target} ${iface} ${libraries})
		FOREACH(library ${libraries})
			IF( TARGET ${library} )
				get_property(type TARGET ${library} PROPERTY TYPE)
				IF(${type} STREQUAL "OBJECT_LIBRARY")
					# Library is an object library target
					## Need to add the interface libraries manually, since that is apparently not done
					IF(${iface} STREQUAL "PUBLIC" OR ${iface} STREQUAL "PRIVATE")
						target_link_libraries(${target} PRIVATE $<TARGET_PROPERTY:${library},INTERFACE_LINK_LIBRARIES> )
					ENDIF()
				ENDIF()
			ENDIF()
		ENDFOREACH()
	ENDIF()
ENDFUNCTION()
