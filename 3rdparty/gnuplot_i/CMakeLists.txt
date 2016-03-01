add_library(gnuplot_interface gnuplot_i.cpp)
IF ( ${MORPHEUS_OS} STREQUAL "WIN32")
	target_link_libraries( gnuplot_interface )
ENDIF ()
