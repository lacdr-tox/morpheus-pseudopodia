FUNCTION(list_prepend IN prefix OUT)
	SET(_list "")
	FOREACH(entry ${${IN}})
		LIST(APPEND _list "${prefix}${entry}")
	ENDFOREACH()
	SET(${OUT} ${_list} PARENT_SCOPE)
ENDFUNCTION()

FUNCTION(target_sources_relpaths target iface sources)
	IF (CMAKE_VERSION VERSION_LESS 3.13)
		get_target_property( target_path ${target} SOURCE_DIR)
		FOREACH(entry ${${sources}})
			target_sources(${target} ${iface} ${CMAKE_CURRENT_SOURCE_DIR}/${entry})
		ENDFOREACH()
	ELSE()
		target_sources(${target} ${iface} ${sources})
	ENDIF()
ENDFUNCTION()
