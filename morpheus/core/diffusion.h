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

#ifndef DIFFUSION_H
#define DIFFUSION_H

#include "interfaces.h"
#include "field.h"
#include "membrane_property.h"
#include "focusrange.h"

/**
\defgroup ML_Diffusion Diffusion
\ingroup ML_Field ML_MembraneProperty ContinuousProcessPlugins

Simulation of spatially homogeneous \b Diffusion is implemented for \ref ML_Field  and \ref ML_MembraneProperty.

Diffusion can be given by a diffusion \b rate or defined to be \b well-mixed, which corresponds to infinitely fast diffusion. Diffusion on membranes is not performed on the actual shape but approximated on a sphere matching the cell's volume.

- \b rate: diffusion coefficient [(node length)² per (global time)]  (! does not yet scale with a \b time-scaling of defined for a corresponding ODE \ref ML_System)
- \b well-mixed (optional): if true, homogenizes the scalar field. Requires rate=0.

**/
class MembranePropertySymbol;

/** @brief Diffusion plugin reading the XML specifivcation and wrapping the difffusion methods implemented in the \ref PDE_Layer of \ref ML_Field or \ref ML_MembraneProperty. **/
class Diffusion : public ContinuousProcessPlugin
{
private:
	shared_ptr<const Field::Symbol> pde_field;
	shared_ptr<const MembranePropertySymbol> mem_field;
	
// 	shared_ptr<PDE_Layer> pde_layer;
// 	CellMembraneAccessor membrane_accessor;
	static double membrane_length_2d(double area);
	static double membrane_length_3d(double volume);
	double (*membrane_length)(double);
	string symbol_name;
	double current_step_size;
public:
	Diffusion(Symbol field);
	void init(const Scope* scope) override;
	void prepareTimeStep(double step_size)  override { current_step_size = step_size; };
	void executeTimeStep() override;
	std::string XMLName() const override;
    XMLNode saveToXML() const override;
};

#endif // DIFFUSION_H
