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
	add_subdirectory(${subdir})
endforeach(subdir)
