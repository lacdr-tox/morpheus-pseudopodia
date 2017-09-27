add_subdirectory(muParser)
add_subdirectory(xmlParser)
add_subdirectory(tiny-process)
add_subdirectory(gnuplot_i)
IF(MORPHEUS_GUI)
        add_subdirectory(qtsingleapp)
ENDIF(MORPHEUS_GUI)

