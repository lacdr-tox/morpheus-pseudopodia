  
  SET(CPACK_PACKAGE_EXECUTABLES "${GUI_EXEC_NAME};Morpheus")
  SET(CPACK_PACKAGE_FILE_NAME "Morpheus-${PROJECT_VERSION}")
  SET(CPACK_PACKAGE_INSTALL_REGISTRY_KEY morpheus)
  SET(CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_NAME} ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}")
  # SET(CPACK_NSIS_CONTACT ${CPACK_PACKAGE_CONTACT})
  SET(CPACK_NSIS_URL_INFO_ABOUT ${CPACK_PACKAGE_HOMEPAGE_URL})

  # RTF update: pandoc -s -f markdown -t rtf LICENSE.md | iconv -f LATIN1 -t UTF-8 - > LICENSE.rtf
  STRING(REPLACE ".md" ".rtf" CPACK_RESOURCE_FILE_LICENSE "${CPACK_RESOURCE_FILE_LICENSE}")

  # create Start Menu links (application, uninstaller and website)
  SET(CPACK_NSIS_MENU_LINKS "${GUI_EXEC_NAME}" "${CPACK_PACKAGE_NAME}")
  # install/uninstall Desktop item
  SET(CPACK_NSIS_EXTRA_INSTALL_COMMANDS
    "CreateShortCut \\\"$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\Morpheus Website.lnk\\\" \\\"https:\\\\\\\\morpheus.gitlab.io\\\" \n
     WriteRegStr HKCU \\\"Software\\\\Morpheus\\\\Morpheus\\\\local\\\" \\\"executable\\\" \\\"$INSTDIR\\\\morpheus.exe\\\" \n
     WriteRegStr HKCU \\\"Software\\\\Classes\\\\morpheus\\\" \\\"URL:Morpheus Model Ressource Protocol\\\" \n
     WriteRegStr HKCU \\\"Software\\\\Classes\\\\morpheus\\\\Shell\\\" \\\"open\\\" \n
     WriteRegStr HKCU \\\"Software\\\\Classes\\\\morpheus\\\\Shell\\\\open\\\\command\\\" \\\"$INSTDIR\\\\morpheus-gui.exe \\\\\\\"%1\\\\\\\"  \\\"
     WriteRegStr HKCU \\\"Software\\\\Classes\\\\morpheus\\\\DefaultIcon\\\"  \\\"$INSTDIR\\\\morpheus-gui.exe,1\\\"
     
  ")
  # CreateShortCut \\\"$DESKTOP\\\\${M_APP_NAME}.lnk\\\" \\\"$INSTDIR\\\\${GUI_EXEC_NAME}\\\" \n
  SET(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS
    "Delete \\\"$DESKTOP\\\\${CPACK_PACKAGE_NAME}.lnk\\\" \n
	 Delete \\\"$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\Morpheus Website.lnk\\\" \n
	 DeleteRegKey HKCU \\\"Software\\\\Morpheus\\\" \n
	 DeleteRegKey HKCU \\\"Software\\\\Classes\\\\morpheus\\\\Shell\\\\open\\\\command\\\"
	 DeleteRegKey HKCU \\\"Software\\\\Classes\\\\morpheus\\\\Shell\\\\open\\\"
	 DeleteRegKey HKCU \\\"Software\\\\Classes\\\\morpheus\\\\Shell\\\"
	 DeleteRegKey HKCU \\\"Software\\\\Classes\\\\morpheus\\\\DefaultIcon\\\"
	 DeleteRegKey HKCU \\\"Software\\\\Classes\\\\morpheus\\\"
    ")

  # name as it appears in the Add/Remove Software panel
  SET(CPACK_NSIS_INSTALLED_ICON_NAME "Morpheus")
  # installation directory of morpheus-GUI need NOT be set in the PATH
  SET(CPACK_NSIS_MODIFY_PATH "OFF")

  # icons ICO for the installer/uninstaller and BMP 'branding image' displayed within installer and uninstaller.
  SET(CPACK_NSIS_MUI_ICON "${PROJECT_SOURCE_DIR}/gui/icons/win/morpheus.ico")
  SET(CPACK_NSIS_MUI_UNIICON "${PROJECT_SOURCE_DIR}/gui/icons/win/morpheus.ico")

