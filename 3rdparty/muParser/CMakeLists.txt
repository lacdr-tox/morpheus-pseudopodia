SET(mu_source
	muParser.cpp
	muParserBase.cpp
	muParserBytecode.cpp
	muParserCallback.cpp
	muParserError.cpp
	muParserInt.cpp
	muParserTokenReader.cpp
)

ADD_LIBRARY( muParser ${mu_source} )
