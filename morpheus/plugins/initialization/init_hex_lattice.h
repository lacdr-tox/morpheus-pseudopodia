#ifndef INITCELLLATICE_H
#define INITCELLLATICE_H

#include "core/interfaces.h"
#include "core/celltype.h"

/** \defgroup InitHexLattice
\ingroup ML_Population
\ingroup InitializerPlugins
\brief Initializes lattice of cell for CA-like models

Initializes a population of cells in which each lattice site is occupied with exactly 1 cell. Useful to populate CA-like models.

Consequently, each cell has area (2D) or volume (3D) \f$ v_{\sigma}=1 \f$. 

Basically a shorthand for the same functionality of InitRectangle.

\section Example
Populate a lattice with cells:
\verbatim
<InitHexLattice />
\endverbatim

Equivalent to InitRectangle for a 20x20 lattice:
\verbatim
<InitRectangle mode="regular" numberOfCells="400">
	<Dimensions size="20,20,0" origin="0.0, 0.0, 0.0"/>
</InitRectangle>
\endverbatim
*/


class InitHexLattice : public Population_Initializer
{
private:
// 	XMLNode stored_node;
	CPM::CELL_ID makeCell(VINT pos, vector<VINT> nbh, CellType* ct);
	enum Direction { LEFT, RIGHT };
	PluginParameter2<Direction, XMLNamedValueReader,DefaultValPolicy> mode;
	PluginParameter2<double, XMLValueReader,DefaultValPolicy> randomness;
public:
	InitHexLattice();
	DECLARE_PLUGIN("InitHexLattice");
	vector<CPM::CELL_ID> run(CellType* ct) override;
};

#endif
