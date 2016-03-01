//////
//
// This file is part of the modelling and simulation framework 'Morpheus',
// and is made available under the terms of the BSD 3-clause license (see LICENSE
// file that comes with the distribution or https://opensource.org/licenses/BSD-3-Clause).
//
// Authors:  Joern Starruss and Walter de Back
// Copyright 2009-2016, Technische Universität Dresden, Germany
//
//////

#include <string>
#include <cstring>
#include <map>
#include <cctype>
#include <iostream>
#include <sstream>

#ifndef ParseArg
#define ParseArg

typedef std::map< std::string, std::string, std::less<std::string> > StringMap;

StringMap ParseArgv(int argc, char *argv[] );

#endif
