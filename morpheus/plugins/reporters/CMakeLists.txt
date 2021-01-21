set(plugin_src 
	
	neighborhood_reporter.cpp
	neighborhood_vector_reporter.cpp
	mapper.cpp
	vector_mapper.cpp
	motility_reporter.cpp

#	population_reporter.cpp
)

target_sources_relpaths(MorpheusCore PRIVATE plugin_src)
