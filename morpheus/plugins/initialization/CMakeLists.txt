set(plugin_src

	init_cell_lattice.cpp
	init_cell_objects.cpp
	init_circle.cpp
	init_hex_lattice.cpp
	init_rectangle.cpp
	init_voronoi.cpp
	tiff_reader.cpp
	init_poisson_disc.cpp
)

target_sources_relpaths(MorpheusSIM PRIVATE plugin_src)
