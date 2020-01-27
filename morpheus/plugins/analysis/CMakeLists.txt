set(plugin_src

	cell_tracker.cpp
	clustering_tracker.cpp
	contact_logger.cpp
    displacement_tracker.cpp
	external.cpp
	gnuplotter.cpp
	histogram_logger.cpp
	logger2.cpp
	# membrane_logger.cpp
	tiff_plotter.cpp
	vtk_plotter.cpp
	
#	NetworkLogger.cpp
#	svgPlotter.cpp
#	vtkPlotter.cpp
#	spatial_correlation.cpp
#    nematicTensorLogger.cpp
)


IF (GRAPHVIZ_FOUND) 
	target_include_directories(MorpheusCore PRIVATE ${GRAPHVIZ_INCLUDE_DIR} )
ENDIF()
list(APPEND plugin_src dependency_graph.cpp )

target_sources_relpaths(MorpheusCore PRIVATE plugin_src)

