FIND_PACKAGE(Qt5 COMPONENTS Core Gui Widgets Network REQUIRED)
### Include Qt directories  *  Add Qt standard Defines  *  Create QT_LIBRARIES variable, including all required libs 

#INCLUDE(${QT_USE_FILE})
#ADD_DEFINITIONS(${QT_DEFINITIONS})

SET(SOURCES
	qtlocalpeer.cpp
	qtsingleapplication.cpp
)
SET(HEADERS
	qtlocalpeer.h
        qtsingleapplication.h
)
SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fPIC" )

INCLUDE_DIRECTORIES(${Qt5Gui_INCLUDE_DIRS} ${Qt5Core_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${Qt5Network_INCLUDE_DIRS} ${Qt5Widgets_INCLUDE_DIRS})


QT5_WRAP_CPP( HEADERS_MOC ${HEADERS} )

ADD_LIBRARY(qtsingleapp ${SOURCES} ${HEADERS_MOC} )
