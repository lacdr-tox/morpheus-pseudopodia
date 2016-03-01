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

#ifndef FLUIDSECRETION_H
#define FLUIDSECRETION_H

#include "core/interfaces.h"
#include "core/celltype.h"
#include "core/pluginparameter.h"
#include <core/cell_membrane_accessor.h>
#include <core/cell_property_accessor.h>

class FluidSecretion : public TimeStepListener
{
private:
	// variable and functions declared here
	CellType* celltype;
	
	// name of fluid celltype
	string fluid_celltype_name;
	shared_ptr<CellType> fluid_ct;
	
	string cellmembrane_symbol_str; // symbol-ref to membrane property
	CellMembraneAccessor cellmembrane;
	
	string targetvolume_str;
	CellPropertyAccessor<double> targetvolume;
	
	
	bool symbolic_rate;
	SymbolAccessor<double> sym_rate;
	shared_ptr<Function> fct_rate;
	
// 	uint createFluidCell(void );
	double angle2d(VDOUBLE v1, VDOUBLE v2);
	
public:
	FluidSecretion();
	DECLARE_PLUGIN("FluidSecretion");
	
	virtual void loadFromXML (const XMLNode);
	virtual void init ();
	virtual void executeTimeStep ();
    virtual set< string > getDependSymbols();
    virtual set< string > getOutputSymbols();
};

#endif // FLUIDSECRETION_H
