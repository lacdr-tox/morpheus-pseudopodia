set(plugin_src 

	cell_division.cpp
	cell_death.cpp
	add_cell.cpp
	change_celltype.cpp
	insert_medium.cpp
	gab_pseudopodia.cpp
	pseudopod.cpp
	# distance_transform.cpp
	# example_plugin.cpp
)

target_sources_relpaths(MorpheusCore PRIVATE plugin_src)
