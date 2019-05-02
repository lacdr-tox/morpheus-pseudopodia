set(plugin_src 

	./volume_constraint.cpp
	./surface_constraint.cpp
	./length_constraint.cpp
	./connectivity_constraint.cpp
)

target_sources_relpaths(MorpheusSIM PRIVATE plugin_src)
