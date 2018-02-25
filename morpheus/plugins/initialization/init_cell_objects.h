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

#ifndef INITCELLOBJECTS_H
#define INITCELLOBJECTS_H

#include "core/interfaces.h"
#include "core/celltype.h"

/** \defgroup InitCellObjects
 * \ingroup ML_Population
\ingroup InitializerPlugins
\brief Initialize a population of cells with predefined geometrical objects in a regular lattice

Initializes cells with defined shapes in 2D and 3D, arranged in a regular fashion. 

Supported shapes (2D/3D) are: square / box, circle / sphere, ellipse / ellipsoid, bar / cylinder.

\b mode (default=distance): Specified how conflicts are resolved in case multiple objects claim the same lattice node. 
  - order: first object is assigned
  - distance: closest object is assigned

\b Arrangement: the layout of the cell objects 
- \b repetitions: (x,y,z) vector specifying the number of objects to create along x,y,z axis (in units of object number).
- \b displacements: (x,y,z) vector specifying the distance between objects along x,y,z axis (in units of lattice sites). 
- \b random_displacement: (default=0.0): Optional value specifying a random displacement of objects from the regular arrangement (in units of lattice sites).

\b Object: specify the type and size of geometrical object
- \b Point: 
  + point: position of first point
- \b Box: specified by origin and size
  + origin: position of bottomleft position of first object
  + size: object size along x, y, z directions
- \b Sphere: specified by center and radius
  + center: center of initial object
  + radius: radius of sphere
- \b Ellipsoid: specified by center and length of semi-principal axes
  + center: center of initial object
  + axes: length of semi-principle axes along x, y, z directions
- \b Cylinder: 
  + center: center of cylinder
  + radius: radius of cylinder
  + orientation: orient cylinder along x, y, or z directions
  + center2 (optional): for oblique cylinders, a second center can be specified

\section Examples

Initializing a single circle or sphere:
\verbatim
<InitCellObjects mode="distance">
	<Arrangement repetitions="1 1 1" displacements="0 0 0">
		<Object>
			<Sphere center="50 50 0" radius="50"/>
		</Object>
	</Arrangement>
</InitCellObjects>
\endverbatim

Initializing 6*6 circles or spheres:
\verbatim
<InitCellObjects mode="distance">
	<Arrangement repetitions="6 6 0" displacements="10 10 0">
		<Object>
			<Sphere center="10 10 0" radius="10"/>
		</Object>
	</Arrangement>
</InitCellObjects>
\endverbatim

Hexagonal tesselation:
choose lattice size (repetitions.x * displacements.x, repetitions.y * displacements.y, 0)
\verbatim
<InitCellObjects mode="distance">
	<Arrangement repetitions="16 14 0" displacements="32 40 0">
		<Object>
			<Sphere radius="30" center="16 16 0"/>
		</Object>
	</Arrangement>
	<Arrangement repetitions="16 14 0" displacements="32 40 0">
		<Object>
			<Sphere radius="30" center="32 40 0"/>
		</Object>
	</Arrangement>
</InitCellObjects>
\endverbatim

Rhombic dodecahedrons
choose lattice size (repetitions.x * displacements.x, repetitions.y * displacements.y, repetitions.z * displacements.z)
\verbatim
<InitCellObjects mode="distance">
	<Arrangement repetitions="5 4 3" displacements="32 48 48">
		<Object>
			<Sphere radius="21" center="16 16 16"/>
		</Object>
	</Arrangement>
	<Arrangement repetitions="5 4 3" displacements="32 48 48">
		<Object>
			<Sphere radius="21" center="32 40 16"/>
		</Object>
	</Arrangement>
	<Arrangement repetitions="5 4 3" displacements="32 48 48">
		<Object>
			<Sphere radius="21" center="32 24 40"/>
		</Object>
	</Arrangement>
	<Arrangement repetitions="5 4 3" displacements="32 48 48">
		<Object>
			<Sphere radius="21" center="16 48 40"/>
		</Object>
	</Arrangement>
</InitCellObjects>
\endverbatim

*/

bool more_double (const pair<double, int>& i, const pair<double, int>& j);

class InitCellObjects : public Population_Initializer
{
private:
	enum Mode { ORDER, DISTANCE };
	enum OType { POINT, BOX, SPHERE, ELLIPSOID, CYLINDER};
	enum Orientation  { X, Y, Z };

	PluginParameter2<Mode, XMLNamedValueReader, DefaultValPolicy> mode;

	double random_displacement;
	shared_ptr<const Lattice> lattice;


	struct CellObject{
		int id;
		VDOUBLE center;
		VDOUBLE center2; // only used for oblique cylinders (i.e. cylinders that are not aligned to x,y,z axis)
		VDOUBLE axes; // only used ellipes and ellipsoids
		VDOUBLE focus1; // only used ellipes and ellipsoids
		VDOUBLE focus2; // only used ellipes and ellipsoids
		bool oblique;
		double radius;
		VDOUBLE origin;
		VDOUBLE boxsize;
		OType type;
		Orientation orientation;
	};
	
	struct Candidate{
		int index;
		VDOUBLE distance;
		double abs_distance;
	};

	
// 	Mode mode;
	vector<CellObject> cellobjects;
	int setNodes(CellType* ct);
	
	//void arrangeObject( CellObject c, vector<CellObject>& objectlist, VDOUBLE displacement, VINT repetitions);
	void arrangeObjectCombinatorial( CellObject c, vector<CellObject>& objectlist, VDOUBLE displacement, VINT repetitions);
	CellObject getObjectProperties(const XMLNode oNode);
	
	void setFociEllipsoid(CellObject& co);
	bool insideEllipsoid(VDOUBLE point, VDOUBLE center, VDOUBLE axes);
	double distanceToLineSegment(VDOUBLE p, VDOUBLE l1, VDOUBLE l2);

	double distancePointToEllipse(double e0, double e1, double y0, double y1, double& x0, double& x1);
	double distancePointToEllipsoid(double e0, double e1, double e2, double y0, double y1, double y2, double& x0, double& x1, double& x2);
	
	void convertPos(string str_Pos);
	bool createNode(VINT newPos, CellType* ct);
	int calculateLines(CellType* ct);

public:
	InitCellObjects();
	DECLARE_PLUGIN("InitCellObjects");

	void loadFromXML(const XMLNode, Scope* scope) override;
	vector<CPM::CELL_ID> run(CellType* ct) override;

};

#endif //INITSPHERECELL_H
