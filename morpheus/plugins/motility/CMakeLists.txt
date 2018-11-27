set(plugin_src

# CPM motility
	chemotaxis.cpp
	directed_motion.cpp
	persistent_motion.cpp
	freeze_motion.cpp
	protrusion.cpp
	star_convex.cpp

 # non-CPM motility
	flipcell_motion.cpp
	margolus_diffusion.cpp

  PARENT_SCOPE)
