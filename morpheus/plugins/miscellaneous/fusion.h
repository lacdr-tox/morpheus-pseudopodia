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

#ifndef FUSION_H
#define FUSION_H

#include "core/interfaces.h"
#include "core/celltype.h"
#include "core/pluginparameter.h"

class Fusion : public TimeStepListener
{
private:
	// variable and functions declared here
	CellType* celltype;
	
	string targetvolume_str;
	CellPropertyAccessor<double> targetvolume;
public:
	DECLARE_PLUGIN("Fusion");
	Fusion();
	virtual void loadFromXML (const XMLNode);
	virtual void init ();
	virtual void executeTimeStep();
	virtual set< string > getDependSymbols();
	virtual set< string > getOutputSymbols();
};

#endif // FUSION_H
