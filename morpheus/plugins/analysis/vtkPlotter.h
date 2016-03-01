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

#ifndef VTKPLOTTER_H
#define VTKPLOTTER_H

#include "core/simulation.h"
#include "core/celltype.h"
#include "core/symbol_accessor.h"
#include <sstream>
#include <fstream>

using namespace SIM;

/** \defgroup vtkPlotter
\ingroup analysis_plugins

\section Description
vtkPlotter is a plugin which creates vtk-files from every interval of the Simulation.
These files illustrate the state of simulation and can be easily viewed in paraview or other tools.

Additionally it is possible to name some PDE-layers.
For the layers vtkPlotter will create a separate vtk-file, which can be loaded by paraview too, so you can see the cells and layers in one view.

\section Example
\verbatim
<Analysis>
	<vtkPlotter interval="100"/>
</Analysis>
\endverbatim

\verbatim
<Analysis>
	<vtkPlotter interval="100">
		<Layer pde_name="fgf"/>
		<Layer pde_name="chemoattractant"/>
	</vtkPlotter>
</Analysis>
\endverbatim
 */

class vtkPlotter : public Analysis_Listener
{
	public:
		DECLARE_PLUGIN("vtkPlotter");

		virtual void init(double time);
		virtual void notify(double time);
		virtual void finish(double time);

		void loadFromXML(const XMLNode);
		virtual set< string > getDependSymbols();

		vtkPlotter();
		~vtkPlotter();
		
	private:
		uint plot_number;
		VINT latticeDim;
		stringstream vtkText;
		
		
		vector< shared_ptr<const CellType> > vec_ct;
		shared_ptr<const CPM::LAYER> cpm_layer;

		struct VTK_CellProperty{
			SymbolAccessor<double> sa;
			string symbolref;
			string name;
		};
		vector<VTK_CellProperty> properties;
		
		struct VTK_PDE{
			shared_ptr< PDE_Layer > pde;
			SymbolAccessor<double> sa;
			string symbolref;
			string name;
		};
		vector<VTK_PDE> pdes;




/*		uint i_anzNodes, i_anzCells;
  		void vtk_header();
		void vtk_points();
		void vtk_cells();
		void vtk_cell_types();
		void vtk_point_data();
		void vtk_cell_data();
		void create_vtk(uint mcs);
		set<VINT,less_VINT> getBoundaryNodes( const Cell& cell );
*/
		
		void writePDELayer(double time);
		void writeCPMLayer(double time);
  
  
};

#endif
