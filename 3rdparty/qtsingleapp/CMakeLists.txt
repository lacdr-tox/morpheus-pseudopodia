FIND_PACKAGE(Qt5 COMPONENTS Widgets Network REQUIRED)
### Include Qt directories  *  Add Qt standard Defines  *  Create QT_LIBRARIES variable, including all required libs 
SET(CMAKE_AUTOMOC ON)
SET(SOURCES
	qtlocalpeer.cpp
	qtsingleapplication.cpp
)

ADD_LIBRARY(qtsingleapp ${SOURCES} ${HEADERS_MOC} )
target_link_libraries(qtsingleapp Qt5::Network Qt5::Widgets )
