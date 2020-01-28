  SET(CPACK_MONOLITHIC_INSTALL 1)
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

