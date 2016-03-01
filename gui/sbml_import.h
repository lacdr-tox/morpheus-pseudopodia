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

#ifndef SBML_IMPORT_H
#define SBML_IMPORT_H

#include "morpheus_model.h"
#ifndef HAVE_LIBSBML
namespace SBMLImporter {
	const bool supported = false;
	inline QSharedPointer<MorphModel> importSBML() { return QSharedPointer<MorphModel>(); };
}
#else // HAVE_LIBSBML
#include "sbml_converter.h"

#endif

#endif // SBML_IMPORT_H
