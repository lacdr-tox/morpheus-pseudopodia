set(plugin_src 

	add_on_adhesion.cpp
	homophilic_adhesion.cpp
	heterophilic_adhesion.cpp
	
	#anisotropic_adhesion.cpp
)

target_sources_relpaths(MorpheusCore PRIVATE plugin_src)
