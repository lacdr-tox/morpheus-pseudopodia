set(plugin_src

# CPM motility
	chemotaxis.cpp
	haptotaxis.cpp
	directed_motion.cpp
	persistent_motion.cpp
	freeze_motion.cpp
	protrusion.cpp
	star_convex.cpp
	

 # non-CPM motility
# 	flipcell_motion.cpp
# 	margolus_diffusion.cpp
)

target_sources_relpaths(MorpheusCore PRIVATE plugin_src)

# if (MORPHEUS_TESTS)
# 	add_subdirectory(tests)
# endif()
