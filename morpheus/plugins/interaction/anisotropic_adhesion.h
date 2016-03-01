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

#ifndef COOP_ADHESION_H
#define COOP_ADHESION_H

#include "core/interfaces.h"
//#include "core/pluginparameter.h"
#include "core/symbol.h"
#include "core/symbol_accessor.h"

class Anisotropic_Adhesion : public Interaction_Addon
{
	private:
		string orientation_str;
		string strength_str;
		string function_str;
		
		SymbolAccessor<double> orientation_rad;
		SymbolAccessor<VDOUBLE> orientation_vec;
		bool use_vectors;
		SymbolAccessor<double> function;
		SymbolAccessor<double> strength;
		
		string angle_str;
		SymbolRWAccessor<double> angle;
		shared_ptr<Function> fct_function;
		
	public:
		DECLARE_PLUGIN("AnisotropicAdhesion");
		virtual double interaction(CPM::STATE s1, CPM::STATE s2) ;
		virtual void init();
		virtual void loadFromXML(const XMLNode xNode);
		double computeAdhesive(CPM::STATE s) ;
};

#endif // COOP_ADHESION_H
