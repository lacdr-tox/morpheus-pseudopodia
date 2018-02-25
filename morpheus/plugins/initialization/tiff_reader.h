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

#ifndef TIFFREADER_H
#define TIFFREADER_H

#include "core/interfaces.h"
#include "core/celltype.h"
#include "core/field.h"
#include "tiffio.h"
#include <sys/time.h>

/** \defgroup TIFFReader
 * \ingroup ML_Population
\ingroup InitializerPlugins

\section Description
TIFFReader loads a configuration of a cell population or gradient field from a TIFF image file.

Cell populations
----------------
For cell populations, a cell will be created for every unique value in the image such that each pixel/voxel with a specific value is be part of a unique cell.

If keepIDs=true, the cell IDs will be equivalent to the image value.

Fields
----------------
For gradient fields, image values correspond to concentrations.

- From 64 bit images, double precision values are used directly and may be scaled by scaleToMax.
- From 32 bit images, floating point values are used directly and may be scaled by scaleToMax.
- From 16 bit images, uint16 values are divided by \f$2^16=65536\f$, and may be scaled by scaleToMax.
- From 8 bit images, uint8 values are divided by \f$2^8=256\f$, and may be scaled by scaleToMax.

\section Notes
- Multipage TIFF images (stacks) are supported. 
- LZW compressed TIFF files are supported.
- TIFF image must have 8 (uint8), 16 (uint16), 32 (float), 64 (double) bit format.
- TIFF image should be grey scale (color coded). Otherwise libTIFF will yield error ''.

\section Input
Required
--------
- *filename*: Path (string) to TIFF file.

Optional
--------
- *offset* (default="0 0 0"): X,Y,Z Coordinates to shift TIFF image with respect to simulation lattice
- *keepIDs* (default="false"): Boolean specifying whether to keep the cell IDs as given in TIFF image (only relevant for loading Cell configurations)
- *scaleToMax* (default="1.0"): Decimal specifying how to scale the values in TIFF to gradient Field (only relavant for loading Fields).

\section Reference
N.A. 

\section Examples

\verbatim
<TIFFReader filename="image.tif" />
\endverbatim

// loading an image with an offset
\verbatim
<TIFFReader filename="/home/user/path/image.tif" offset="10 10 0" />
\endverbatim

// loading a CellPopulation with offset
\verbatim
<TIFFReader filename="image.tif" keepIDs="true" />
\endverbatim

// loading a Field and scaling the concentrations by factor 100
\verbatim
<TIFFReader filename="image.tif" scaleToMax="100.0" />
\endverbatim
*/

class TIFFReader : public Population_Initializer, public Field_Initializer
{
private:
	PluginParameter2<string, XMLValueReader, RequiredPolicy> filename;
	PluginParameter2<VINT, XMLValueReader, DefaultValPolicy> offset;
	PluginParameter2<bool, XMLValueReader, DefaultValPolicy> keepIDs;
	PluginParameter2<double, XMLValueReader, DefaultValPolicy> scaleToMax;
	
	enum { CELLS, PDE } mode;
	
	CellType* celltype;
	PDE_Layer* pde_layer;
	CPM::CELL_ID empty_state;
	vector<CPM::CELL_ID> cells_created;

	map<uint32, uint> color_to_cellid; // maps color to cellID
	uint skipped_nodes, created_nodes, created_cells;
	
	bool loadTIFF(void);
	void addNode(VINT pos, uint32 color);
	//void setValue(VINT pos, int32 color);

public:
	TIFFReader();
	DECLARE_PLUGIN("TIFFReader");
	
	vector<CPM::CELL_ID> run(CellType* ct) override;
	bool run(PDE_Layer* pde) override;
	
};

#endif // TIFFREADER_H
