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

#ifndef ROD_VORTEXTURNER_H
#define ROD_VORTEXTURNER_H

#include "core/interfaces.h"
#include "core/super_celltype.h"
#include "core/pluginparameter.h"


class VortexTurner : public Celltype_MCS_Listener
{
private:
	SuperCT* celltype;
	CellPropertyAccessor<VDOUBLE> orientation;
	CellPropertyAccessor<double> reversed;
	string reversal_name, orientation_name;
	int check_frequency;
public:
	DECLARE_PLUGIN("VortexTurner");
	
    VortexTurner();
    virtual void mcs_notify(uint mcs);
    virtual void init(CellType* p);
    virtual void loadFromXML(const XMLNode xNode );
};

#endif // ROD VORTEXTURNER_H
