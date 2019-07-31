set(plugin_src 

	cell_division.cpp
	cell_death.cpp
	add_cell.cpp
	change_celltype.cpp
	insert_medium.cpp
	# distance_transform.cpp
	# example_plugin.cpp
)

target_sources_relpaths(MorpheusSIM PRIVATE plugin_src)
