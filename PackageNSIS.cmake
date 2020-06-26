  SET(CPACK_MONOLITHIC_INSTALL 1)
  
  SET(MORPHEUS_EXE ${PROJECT_BINARY_DIR}/morpheus/morpheus.exe)
 

	
	#	IF (GRAPHVIZ_FOUND)
	#	SET(GRAPHVIZ_RUNTIME
	#		${GRAPHVIZ_ROOT}/bin/config6
	#		${GRAPHVIZ_ROOT}/bin/libexpat.dll
	#		${GRAPHVIZ_ROOT}/bin/ltdl.dll
	#		${GRAPHVIZ_ROOT}/bin/zlib1.dll
	#		${GRAPHVIZ_ROOT}/lib/release/dll/ann.dll
	#		${GRAPHVIZ_ROOT}/lib/release/dll/cdt.dll
	#		${GRAPHVIZ_ROOT}/lib/release/dll/cgraph.dll
	#		${GRAPHVIZ_ROOT}/lib/release/dll/gvc.dll
	#		${GRAPHVIZ_ROOT}/lib/release/dll/gvplugin_core.dll
	#		${GRAPHVIZ_ROOT}/lib/release/dll/gvplugin_dot_layout.dll
	#		${GRAPHVIZ_ROOT}/lib/release/dll/gvplugin_gd.dll
	#		${GRAPHVIZ_ROOT}/lib/release/dll/gvplugin_gdiplus.dll
	#		${GRAPHVIZ_ROOT}/lib/release/dll/gvplugin_neato_layout.dll
	#		${GRAPHVIZ_ROOT}/lib/release/dll/gvplugin_pango.dll
	#		${GRAPHVIZ_ROOT}/lib/release/dll/Pathplan.dll
	#		${GRAPHVIZ_ROOT}/lib/release/dll/vmalloc.dll
	#	)
	#	INSTALL(FILES ${GRAPHVIZ_RUNTIME} DESTINATION . COMPONENT GraphViz)
	#ENDIF()
	
	#	SET(GNUPLOT_ROOT "" CACHE PATH "Set root directory to search for Gnuplot" )
	#	MARK_AS_ADVANCED(GNUPLOT_ROOT)
	
	#message(STATUS "Gnuplot Path is ${GNUPLOT_ROOT}")
	#SET(GNUPLOT_LIBRARIES 
	#	${GNUPLOT_ROOT}/freetype6.dll
	#	${GNUPLOT_ROOT}/intl.dll
	#	${GNUPLOT_ROOT}/libcairo-2.dll
	#	${GNUPLOT_ROOT}/libexpat-1.dll
	#	${GNUPLOT_ROOT}/libfontconfig-1.dll
	#	${GNUPLOT_ROOT}/libgd-2-733361a31aab.dll
	#	${GNUPLOT_ROOT}/libglib-2.0-0.dll
	#	${GNUPLOT_ROOT}/libgmodule-2.0-0.dll
	#	${GNUPLOT_ROOT}/libgobject-2.0-0.dll
	#	${GNUPLOT_ROOT}/libgthread-2.0-0.dll
	#	${GNUPLOT_ROOT}/libiconv-2.dll
	#	${GNUPLOT_ROOT}/libjpeg-8.dll
	#	${GNUPLOT_ROOT}/libpango-1.0-0.dll
	#	${GNUPLOT_ROOT}/libpangocairo-1.0-0.dll
	#	${GNUPLOT_ROOT}/libpangoft2-1.0-0.dll
	#	${GNUPLOT_ROOT}/libpangowin32-1.0-0.dll
	#	${GNUPLOT_ROOT}/libpng14-14.dll
	#	${GNUPLOT_ROOT}/lua51.dll
	#	${GNUPLOT_ROOT}/wxbase28_gcc_custom.dll
	#	${GNUPLOT_ROOT}/wxmsw28_core_gcc_custom.dll
	#	${GNUPLOT_ROOT}/zlib1.dll
	#)
	#INSTALL(FILES ${GNUPLOT_LIBRARIES} DESTINATION . COMPONENT GnuPlot)
	#INSTALL(FILES ${GNUPLOT_ROOT}/gnuplot.exe DESTINATION . COMPONENT GnuPlot)
  
  SET(CPACK_PACKAGE_EXECUTABLES "${GUI_EXEC_NAME};Morpheus")
  SET(CPACK_PACKAGE_FILE_NAME "morpheus-${PROJECT_VERSION}.${CMAKE_SYSTEM_NAME}.b${MORPHEUS_REVISION}")
  SET(CPACK_PACKAGE_INSTALL_REGISTRY_KEY morpheus)
  SET(CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_NAME} ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}")
  # SET(CPACK_NSIS_CONTACT ${CPACK_PACKAGE_CONTACT})
  SET(CPACK_NSIS_URL_INFO_ABOUT ${CPACK_PACKAGE_HOMEPAGE_URL})

  # RTF update: pandoc -s -f markdown -t rtf LICENSE.md  > LICENSE.rtf
  STRING(REPLACE ".md$" ".rtf" CPACK_RESOURCE_FILE_LICENSE "${CPACK_RESOURCE_FILE_LICENSE}")

  # create Start Menu links (application, uninstaller and website)
  SET(CPACK_NSIS_MENU_LINKS "${GUI_EXEC_NAME}" "${CPACK_PACKAGE_NAME}")
  # install/uninstall Desktop item
  SET(CPACK_NSIS_EXTRA_INSTALL_COMMANDS
    "CreateShortCut \\\"$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\Morpheus Website.lnk\\\" \\\"http:\\\\\\\\morpheus.gitlab.io\\\" \n
     WriteRegStr HKCU \\\"Software\\\\Morpheus\\\\Morpheus\\\\local\\\" \\\"executable\\\" \\\"$INSTDIR\\\\morpheus.exe\\\"
  ")
  # CreateShortCut \\\"$DESKTOP\\\\${M_APP_NAME}.lnk\\\" \\\"$INSTDIR\\\\${GUI_EXEC_NAME}\\\" \n
  SET(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS
    "Delete \\\"$DESKTOP\\\\${CPACK_PACKAGE_NAME}.lnk\\\" \n
         Delete \\\"$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\Morpheus Website.lnk\\\"
    ")

  # name as it appears in the Add/Remove Software panel
  SET(CPACK_NSIS_INSTALLED_ICON_NAME "${GUI_EXEC_NAME}")
  # installation directory of morpheus-GUI need NOT be set in the PATH
  SET(CPACK_NSIS_MODIFY_PATH "OFF")

  # icons ICO for the installer/uninstaller and BMP 'branding image' displayed within installer and uninstaller.
  SET(CPACK_NSIS_MUI_ICON "${PROJECT_SOURCE_DIR}/gui/icons/win/morpheus.ico")
  SET(CPACK_NSIS_MUI_UNIICON "${PROJECT_SOURCE_DIR}/gui/icons/win/morpheus.ico")
  #SET(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/gui/icons/win/morpheus.ico")

