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
#include "cell_membrane_accessor.h"


class Diffusion : public ContinuousProcessPlugin
{
private:
	SymbolData::LinkType container_type;
	shared_ptr<PDE_Layer> pde_layer;
	CellMembraneAccessor membrane_accessor;
	static double membrane_length_2d(double area);
	static double membrane_length_3d(double volume);
	double (*membrane_length)(double);
	string symbol_name;
public:
	Diffusion(shared_ptr<PDE_Layer> pde);
	Diffusion(CellMembraneAccessor mem_acc);
	void init(const Scope* scope) override;
	void prepareTimeStep()  override {};
	void executeTimeStep() override;
	std::string XMLName() const override;
    XMLNode saveToXML() const override;
};

#endif // DIFFUSION_H
