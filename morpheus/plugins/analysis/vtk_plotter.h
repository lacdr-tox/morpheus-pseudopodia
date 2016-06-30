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

#include <core/interfaces.h>
#include "core/simulation.h"
#include "gnuplot_i/gnuplot_i.h"
#include "core/plugin_parameter.h"
#include <limits>
#include <sstream>
#include <fstream>

using namespace SIM;
/** \ingroup AnalysisPlugins
\defgroup VtkPlotter
\brief Writes cells and fields to legacy VTK files

Writes cells and fields to legacy VTK files. Currently, only ASCII format is supported and no compression is applied. 

Note: cannot handle hexagonal latices.

- \b time-step: Time interval between saving simulation state to VTK file.
- \b mode (default=binary): Write Vtk files in ascii or binary form. Using binary is faster and results in smaller files.

- \b Channel: defines a symbol to plot in channel, multiple channel are possible
  - \b symbol-ref: Symbol to plot
  - \b celltype (optional): To plot symbol from single cell type (in case of symbol-ref refers to cell Property). If not defined, global scope is assumed.
  - \b outline (default=false): Plot values on cell surface only (e.g. like membraneproperties are drawn). Can be used to visualize cells semi-transparently.
  - \b no-outline (default=false): Do NOT plot values on cell surface. Can be used to separate cells in the image.
  - \b exclude-medium (optional): Medium is not plotted (zero).
   
\section Examples

\verbatim
<VtkPlotter time-step="100">
	<Channel symbol-ref="cell.id"/>
	<Channel symbol-ref="act"/>
</VtkPlotter >
\endverbatim
*/

#include <climits>
template <typename T>
T swap_endian(T u)
{
    static_assert (CHAR_BIT == 8, "CHAR_BIT != 8");

    union
    {
        T u;
        unsigned char u8[sizeof(T)];
    } source, dest;

    source.u = u;

    for (size_t k = 0; k < sizeof(T); k++)
        dest.u8[k] = source.u8[sizeof(T) - k - 1];

    return dest.u;
}


class VtkPlotter : public AnalysisPlugin
{
	enum Mode { ASCII, BINARY };

private:
	PluginParameter2<Mode, XMLNamedValueReader, DefaultValPolicy> mode;
	
	struct Channel{
		PluginParameter2<double, XMLReadableSymbol, RequiredPolicy> symbol;
		PluginParameterCellType< OptionalPolicy > celltype;
		
		PluginParameter2<double, XMLValueReader, OptionalPolicy> min_user;
		PluginParameter2<double, XMLValueReader, OptionalPolicy> max_user;
		double min, max;
		PluginParameter2<bool, XMLValueReader, DefaultValPolicy> scale;
		PluginParameter2<bool, XMLValueReader, DefaultValPolicy> outline;
		PluginParameter2<bool, XMLValueReader, DefaultValPolicy> no_outline;
		PluginParameter2<bool, XMLValueReader, DefaultValPolicy> exclude_medium;
		
		shared_ptr<PDE_Layer> pde_layer; 
		CellMembraneAccessor membrane;
	};
	
		
	struct Plot{
		PluginParameter2<bool, XMLValueReader, DefaultValPolicy> exclude_medium;
		vector< shared_ptr<Channel> > channels;

		bool cropToCell;
		vector< CPM::CELL_ID > cellids;
		PluginParameter2<string, XMLValueReader, RequiredPolicy> cellids_str;
		PluginParameter2<uint, XMLValueReader, DefaultValPolicy> padding;
	};
	Plot plot;
	
	uint plot_number;
	stringstream vtkText;
	
	VINT latticeDim;
	shared_ptr<const CPM::LAYER> cpm_layer;
		
	void writeVTK(double time);
  
public:
	DECLARE_PLUGIN("VtkPlotter");
	
	virtual void loadFromXML(const XMLNode);
	virtual void init(const Scope* scope);
	virtual void analyse(double time);	
	virtual void finish(double time);

	VtkPlotter(){};
	~VtkPlotter(){};
};

#endif
