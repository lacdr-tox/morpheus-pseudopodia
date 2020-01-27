cmake_policy(SET CMP0022 NEW)

add_library(ModelTesting OBJECT model_test.cpp)
target_include_directories(ModelTesting PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries_fixed(ModelTesting PUBLIC MorpheusCore)

FUNCTION(InjectModels target)
	add_library(${target}_ressources)
	
	SET(RESSOURCE_INCLUDES "")
	SET(RESSOURCE_FILE_DESCRIPTORS "")
	
	get_target_property(sources ${target} SOURCES)
	FOREACH(source ${sources})
		file(STRINGS ${source} entries REGEX "ImportFile")
# 		file(READ ${source} data HEX)
# 		message(STATUS ${data})SOURCE
		FOREACH(entry ${entries})
			IF (entry MATCHES "RessourceData ImportFile")
				### Skip the Implementation of the method ...
# 				message(STATUS "${entry} is the method implementation" )
				continue()
			ENDIF()
			string(REGEX MATCH "ImportFile\\((.*)\\)" match ${entry} )
			IF(match)
				SET(MODEL_FILE ${CMAKE_MATCH_1})
				## Remove white space
				string(REGEX REPLACE "[ \t\"]" "" TRIMMED_FILE ${MODEL_FILE})
				SET(MODEL_FILE ${TRIMMED_FILE})
				IF(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${MODEL_FILE})
					message(FATAL_ERROR "Ressource file ${MODEL_FILE} does not exit")
				ENDIF()
				string(REGEX REPLACE "[./]" "_" variable ${TRIMMED_FILE} )
				message(STATUS "Injecting file ${MODEL_FILE} as variable ${variable} " )
				SET(MODEL_RES ${CMAKE_CURRENT_BINARY_DIR}/${variable}.h)

				add_custom_command(OUTPUT ${MODEL_RES} 
								   COMMAND xxd ARGS -i ${MODEL_FILE} ${MODEL_RES} 
								   MAIN_DEPENDENCY ${MODEL_FILE} 
								   DEPENDS ${source}
								   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
								   COMMENT "Moving ${MODEL_FILE} to ressources"
								   )
# 				add_dependencies(${target}_ressources ${MODEL_RES} )
				target_sources(${target}_ressources PRIVATE ${MODEL_RES} )
				LIST(APPEND RESSOURCE_INCLUDES "#include \"${variable}.h\"\n")
# 				LIST(APPEND ressources_init "ressources[\"${TRIMMED_FILE}\"] = ${variable};")
				LIST(APPEND RESSOURCE_FILE_DESCRIPTORS "\t{\"${TRIMMED_FILE}\", ${variable},  ${variable}_len},\n")
			endif()
		ENDFOREACH()
# 		ENDIF()
	ENDFOREACH()

	## Remove trailing comma
	STRING(REGEX REPLACE "[ \n,]+$" "" RESSOURCE_FILE_DESCRIPTORS "${RESSOURCE_FILE_DESCRIPTORS}")
	
	# Configure the ressource source file
	configure_file(${CMAKE_SOURCE_DIR}/morpheus/testing/ressources.cpp.in ${target}_ressources.cpp @ONLY)
	SET(ressources_file  ${CMAKE_CURRENT_BINARY_DIR}/${target}_ressources.cpp)
	
	# Assemble ressources to a target
	target_sources(${target}_ressources PRIVATE ${ressources_file})
	# Add the RessourceManager implementation library target
	target_link_libraries(${target}_ressources PRIVATE ModelTesting) 
	# Append the ressources to the target
	target_link_libraries(${target} PRIVATE ${target}_ressources )
ENDFUNCTION()

# InjectModels(ModelTesting)
