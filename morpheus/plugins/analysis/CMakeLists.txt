set(plugin_src
        
	gnuplotter.cpp
	histogram_logger.cpp
	tiff_plotter.cpp
#    logger.cpp
    clustering_tracker.cpp
    membrane_logger.cpp
	
	logger2.cpp # temporary
	cell_tracker.cpp
	contact_logger.cpp

#	space_time_logger.cpp #obsolete (replaced by logger2)
#	NetworkLogger.cpp
#	svgPlotter.cpp
#	vtkPlotter.cpp
    displacement_tracker.cpp
#	spatial_correlation.cpp
#    membraneLogger.cpp #obsolete (replaced by logger2)
#    nematicTensorLogger.cpp
)

IF (GRAPHVIZ_FOUND) 
	set(plugin_src
		${plugin_src}
		dependency_graph.cpp
	)
ENDIF()

SET(plugin_src ${plugin_src} PARENT_SCOPE)
