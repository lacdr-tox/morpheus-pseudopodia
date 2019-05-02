set(subdirs ${subdirs} 
	analysis
	initialization
	interaction
	miscellaneous
	motility
	reporters
	shape
	#segmented_cell
# 	examples
)


foreach(subdir ${subdirs})
set(plugin_src "")
	add_subdirectory(${subdir})
	foreach(sub_plugin_src ${plugin_src})
		list(APPEND acc_plugin_src ${subdir}/${sub_plugin_src})
	endforeach(sub_plugin_src)
endforeach(subdir)

SET(plugin_src ${acc_plugin_src} PARENT_SCOPE)
