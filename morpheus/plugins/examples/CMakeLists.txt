set(plugin_src
# 	example_simple1.cpp
# 	example_timeavg_reporter.cpp
	example_boundingbox_symbol.cpp
)

# find_package(libXXX)
# IF (libXXX_FOUND) 
	list(APPEND plugin_src
		example_external_symbol.cpp
	)
# ENDIF()

SET(plugin_src ${plugin_src} PARENT_SCOPE)
