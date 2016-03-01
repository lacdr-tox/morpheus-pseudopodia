FIND_PACKAGE(Qt4 COMPONENTS QtCore QtGui QtNetwork REQUIRED)
### Include Qt directories  *  Add Qt standard Defines  *  Create QT_LIBRARIES variable, including all required libs 
INCLUDE(${QT_USE_FILE})
ADD_DEFINITIONS(${QT_DEFINITIONS})

SET(SOURCES
	qtlocalpeer.cpp
	qtsingleapplication.cpp
)
SET(HEADERS
	qtlocalpeer.h
        qtsingleapplication.h
)

QT4_WRAP_CPP( HEADERS_MOC ${HEADERS} )

ADD_LIBRARY(qtsingleapp ${SOURCES} ${HEADERS_MOC} )
