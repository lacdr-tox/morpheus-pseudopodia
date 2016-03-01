SET(mu_source
	muParser.cpp
	muParserBase.cpp
	muParserBytecode.cpp
	muParserCallback.cpp
	muParserError.cpp
	muParserInt.cpp
	muParserTokenReader.cpp
)

IF (HAVE_OBJECT_LINKAGE)
	IF(DEFINED muParser_LIB_DEPENDS)
		UNSET(muParser_LIB_DEPENDS CACHE)
	ENDIF()
	ADD_LIBRARY(muParser OBJECT ${mu_source}  )
ELSE ()
	ADD_LIBRARY(muParser ${mu_source}  )
ENDIF()

