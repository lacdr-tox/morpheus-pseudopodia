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



#include "core/interfaces.h"
#include "core/celltype.h"
#include <core/cell_membrane_accessor.h>
#include <core/cell_property_accessor.h>

class Vesicles : public Celltype_MCS_Listener
{
private:
	// variable and functions declared here
	CellType* celltype;
	
	struct Endo{
	  
	  double cargo;
// 	  double rab4; // recycling
	  double rab5; // sorting
// 	  double rab7; // degradation
// 	  bool status;
	  
	};
	
	vector < vector <Endo> > endosomes_all_cells;
	
	double J_in, s0;
	double k_min, r_fuse, r_fission;
	double delta_t;
	
	ofstream fout;
// 	// name of fluid celltype
// 	string fluid_celltype_name;
// 	shared_ptr<CellType> fluid_ct;
// 	
// 	string cellmembrane_symbol_str; // symbol-ref to membrane property
// 	CellMembraneAccessor cellmembrane;
// 	
// 	string targetvolume_str;
// 	CellPropertyAccessor<double> targetvolume;
// 	
// 	
// 	bool symbolic_rate;
// 	SymbolAccessor<double> sym_rate;
// 	shared_ptr<Function> fct_rate;
// 	
// 	uint createFluidCell(void );
// 	double angle2d(VDOUBLE v1, VDOUBLE v2);
	
public:
	Vesicles();
	DECLARE_PLUGIN("Vesicles");


	virtual void mcs_notify (uint mcs);
	virtual void init (CellType*);
	virtual void loadFromXML (const XMLNode);
};
