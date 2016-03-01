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

#ifndef DIFFERENTIALEQN_H
#define DIFFERENTIALEQN_H

#include "core/interfaces.h"

/** @brief Differential Equation Plugin
 * 
 *  A plain Plugin loading differential  equations from XML.
 *  Evaluation is responisbility of the enclosing "System".
 */

class DifferentialEqn : public Plugin
{
	private:
		string symbol_name;
		string expression;
		CellType* celltype;
		valarray<double> delta_data;
	public:
		DECLARE_PLUGIN("DiffEqn");
		virtual void loadFromXML(const XMLNode );
		/// This init method is called by PDE_Sim
		virtual void init(const Scope* scope);
		string getExpr() { return expression; }
		string getSymbol() { return symbol_name; }
};

#endif // DIFFERENTIALEQN_H
