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

#ifndef TIFFPLOTTER_H
#define TIFFPLOTTER_H

#include <core/interfaces.h>
#include "core/simulation.h"
#include "gnuplot_i/gnuplot_i.h"
#include "core/plugin_parameter.h"
#include <limits>
#include <tiffio.h>

/** \ingroup AnalysisPlugins
\defgroup TiffPlotter
\brief Writes cells and fields to multipage TIFF images

TiffPlotter can write 3D(+time) data to file in multistack multichannel TIFF format.

Writes grayscale TIFF file in 8, 16 or 32bit format.

Multipage images are exported in XYCZT format. 

TiffPlotter supports:
- z-stack 3d images
- multiple channels, see Channel
- 3D+time: with 'timelapse' true, appends image to existing TIFF, see timeslapse
- cropping to specific cell(s), see CropToCell

Note: cannot handle hexagonal latices.

- \b format (default=guess): bitformat of TIFF image: 8 bit (int 0-255), 16 bit (int 0-65536), or 32 bit (float, single precision).
- \b compression (default=true): use lossless LZW compression.
- \b timelapse (default=true): append images to create 2D+time or 3D+time image
- \b OME-header (default=false): write OME-TIFF header with meta-data on TIFF organization (OME=Open Microscopy Environment).

- \b Channel: defines a symbol to plot in channel, multiple channel are possible
  - \b symbol-ref: Symbol to plot
  - \b celltype (optional): To plot symbol from single cell type (in case of symbol-ref refers to cell Property). If not defined, global scope is assumed.
  - \b minimum (optional): User-specified fixed minimum value. If not given, determined by data.
  - \b maximum (optional): User-specified fixed minimum value. If not given, determined by data.
  - \b scale (optional): If true, data is scaled according to min/max values. If false, raw values are used.
  - \b outline (default=false): Plot values on cell surface only (e.g. like membraneproperties are drawn). Can be used to visualize cells semi-transparently.
  - \b no-outline (default=false): Do NOT plot values on cell surface. Can be used to separate cells in the image.
  - \b exclude-medium (optional): Medium is not plotted (zero).
 
- \b CropToCell: plot small TIFF image(s) containing single cell(s)
  - \b padding (optional): number of voxels to add to image around cell volume 
  - \b cell-ids (required): list of cell IDs to plot in the following format:
		comma-separated: 1,2,3,4,5
		dash-seperated:  1-5 
		combined:        1,3-5
  
\section Examples
Write multiple timelapse multichannel images of cell properties 'cell.id' and 'c' to 8-bit TIFF image files for cellsa with IDs between 10 and 15.
\verbatim
<TiffPlotter format="8bit" time-step="100">
	<Channel symbol-ref="cell.id" celltype="cell"/>
	<Channel symbol-ref="c" celltype="cell"/>
	<CropToCell cell-ids="10-15" padding="2"/>
</TiffPlotter>
\endverbatim

Write time-lapse multichannel image to 32-bit TIFF image file. See ExcitableMedium_3D example.
\verbatim
<TiffPlotter timelapse="true" format="32bit" compression="true" time-step="0.5">
	<Channel symbol-ref="u"/>
	<Channel symbol-ref="v"/>
</TiffPlotter>
\endverbatim

*/



using namespace SIM;

class TiffPlotter : public AnalysisPlugin
{
private:
	static int instances;
	int instance_id;
	
	XMLNode stored_node;
	
	//bool OME header;
	PluginParameter2<bool, XMLValueReader, DefaultValPolicy> ome_header;
	//bool timelapse;
	PluginParameter2<bool, XMLValueReader, DefaultValPolicy> timelapse;
	//bool compression;
	PluginParameter2<bool, XMLValueReader, DefaultValPolicy> compression;
	uint format;
	PluginParameter2<uint, XMLNamedValueReader, RequiredPolicy> bitformat;
	
	bool timename;
	bool multichannel;
	
	ofstream file_boundingbox;
	
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
	
	
	struct ImageDescription{
		int images;
		int frames;
		double min;
		double max;
	};
	
	vector< ImageDescription > im_desc;
	
	VINT latticeDim;
	shared_ptr<const CPM::LAYER> cpm_layer;
	
	bool fileExists(string filename);
	string getFileName(CPM::CELL_ID cellid=0);
	vector<CPM::CELL_ID> parseCellIDs(string cell_ids_string);
	void writeTIFF(CPM::CELL_ID cellid = CPM::NO_CELL);

public:
	DECLARE_PLUGIN("TiffPlotter");
	
	virtual void init(const Scope* scope);
	virtual void analyse(double time);	
	virtual void finish(double time);
	
	void loadFromXML(const XMLNode);
	
	TiffPlotter();
	~TiffPlotter();
	

};

#endif //TIFFPLOTTER_H
