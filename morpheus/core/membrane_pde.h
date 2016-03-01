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

#ifndef MEMBRANE_PDE_H
#define MEMBRANE_PDE_H

#include "core/interfaces.h"
#include "core/pde_layer.h"
#include "core/scales.h"

/**
\defgroup MembraneProperty
\ingroup Symbols

Symbol with a variable scalar field, mapped to a cell membrane that associates a scalar value to every lattice site in the domain.

A MembraneProperty is a circular (2D) or spherical (3D) lattice mapped to the surface nodes of a cell using polar coordinates.

- \b value: initial condition for the scalar field. May be given as symbolic expression.

Optionally, a \b Diffusion rate may be specified.

- \b rate: diffusion coefficient
- \b unit (optional): physical unit of diffusion coefficient
- \b well-mixed (optional): if true, homogenizes scalar field. Requires rate=0.

**/

/** Wrapper class that creates a membrane pde from XML data.
 *  The membrane pde is read by the celltype and then included as default membrane
*/
class MembraneProperty : public Plugin
{
private:
	//XMLNode stored_node;
	
	static shared_ptr<const Lattice>  membrane_lattice;
	Length_Scale node_length;
	shared_ptr <PDE_Layer> pde_layer;
	
public:
	DECLARE_PLUGIN("MembraneProperty");
	
	void loadFromXML(const XMLNode );
	
	string getSymbolName( void );
	string getName( void );
	shared_ptr<PDE_Layer> getPDE( void );

	static uint resolution;
	static string resolution_symbol;
	static VINT size;

	static shared_ptr<const Lattice> lattice();
};

#endif 
