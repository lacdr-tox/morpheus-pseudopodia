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

#ifndef MARGOLUSDIFFUSION_H
#define MARGOLUSDIFFUSION_H

#include "core/interfaces.h"
#include "core/celltype.h"

/** \defgroup MargolusDiffusion
\ingroup ML_CellType
\ingroup CellMotilityPlugins
\brief Diffusion operator for CA-like models.

Margolus diffusion is a reversible particle-conserving diffusion operator. 

It uses block-synchronous diuffusion to avoid collision conflicts of particles with random motion. 

Each block consists of a 2x2 neighborhood of cells. Even and odd iterations are distinguished. 
Even iterations use clockwise rotations of particles. In odd iterations, the neighborhood is shifted and particles are rotated counterclockwise.

- \b numsteps: Value (integer > 0) specifying number of Margolus diffusion steps. 

\section Notes
- Only applicable for 2D CA-like models with periodic boundary conditions.

\section Reference
Toffoli, T. and Margolus, N.: 1987, Cellular Automata Machines, MIT Press, Cambridge, London.

\section Example
\verbatim
<Margolus numsteps="4" />
\endverbatim
*/


class MargolusDiffusion: public InstantaneousProcessPlugin {
private:
	PluginParameter2<uint, XMLValueReader, DefaultValPolicy> iterations;
	CellType* celltype;

	bool shifted;
	vector< vector<VINT> > margolus_partitions_normal;
	vector< vector<VINT> > margolus_partitions_shifted;

	bool firstRun;
	void margolus();
	vector<CPM::STATE> getNodes( vector<VINT> cells );

public:
	MargolusDiffusion();
	DECLARE_PLUGIN("MargolusDiffusion");
    void init(const Scope* scope) override;
	void executeTimeStep() override;
};


#endif //MARGOLUSDIFFUSION_H
