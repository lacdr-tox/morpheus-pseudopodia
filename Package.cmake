SET(CPACK_PACKAGE_NAME "Morpheus")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Morpheus - Modeling environment for Multi-Cellular Systems Biology")
SET(CPACK_PACKAGE_DESCRIPTION "Modeling environment for Multi-Cellular Systems Biology
 Modeling and simulation environment for multi-cellular systems biology.
 Includes simulators for ODEs, PDEs and cellular Potts models. 

 Facilitates 
  - Modeling without programming.
  - Model integration into multiscale models by symbolic linking.
  - Import of SBML models.
  - Simulating, sharing and archiving Models with ease.
  - Parameter exploration through (parallel) batch processing.
  - Multi-core concurrency support using OpenMP multithreading")

SET(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR} )
SET(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR} )
SET(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH} )

SET(CPACK_PACKAGE_NAME_TAG "b${MORPHEUS_REVISION}")

SET(CPACK_MAINTAINER "Joern Starruss <joern.starruss@tu-dresden.de>")
SET(CPACK_PACKAGE_VENDOR "Jörn Starruß & Walter de Back (ZIH, TU-Dresden)")
SET(CPACK_PACKAGE_CONTACT ${CPACK_MAINTAINER})
SET(CPACK_PACKAGE_HOMEPAGE_URL "https://morpheus.gitlab.io")

SET(CPACK_PACKAGE_EXECUTABLES  ${GUI_EXEC_NAME}; ${CPACK_PACKAGE_DESCRIPTION_SUMMARY})
SET(CPACK_STRIP_FILES TRUE)
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_NAME}")

# SET(CPACK_RESOURCE_FILE_WELCOME_ "${CMAKE_CURRENT_SOURCE_DIR}/README")

SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.md")

IF( "${MORPHEUS_OS}" STREQUAL "UNIX" AND  NOT "${MORPHEUS_OS}" STREQUAL "APPLE" )
  SET(CPACK_GENERATOR "DEB")
  INCLUDE(PackageDEB.cmake)
ELSEIF("${MORPHEUS_OS}" STREQUAL "APPLE")
  IF (MORPHEUS_RELEASE_BUNDLE)
    SET(CPACK_GENERATOR "Bundle") ## creates a DMG image with a bundle inside
    INCLUDE(PackageBundle.cmake)
  ENDIF()
ELSEIF("${MORPHEUS_OS}" STREQUAL "WIN32")
  SET(CPACK_GENERATOR "NSIS")   ## alternative packagers WIX;IFW")
  INCLUDE (PackageNSIS.cmake)
ENDIF()

SET(CPACK_PROJECT_CONFIG_FILE "${PROJECT_SOURCE_DIR}/packaging/cpack_config.cmake")

INCLUDE(CPack)

