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

/** 
 * \defgroup InitCellObjects
 * \brief Initialize a population of cells with predefined geometrical objects in a regular lattice
 * \ingroup ML_Population
 * \ingroup InitializerPlugins

Initializes cells with defined shapes in 2D and 3D, arranged in a regular fashion. 

Supported shapes (2D/3D) are: square / box, circle / sphere, ellipse / ellipsoid, bar / cylinder.

\b mode (default=distance): Specifies how conflicts are resolved in case multiple objects claim the same lattice node. 
  - order: object with lowest celltype.id (as given by order of CellType definitions) is assigned
  - distance: closest object is assigned

\b Arrangement: the layout of the cell objects 
- \b repetitions: (x,y,z) vector specifying the number of objects to create along x,y,z axes (in units of object number).
- \b displacements: (x,y,z) vector specifying the distance between objects along x,y,z axes (in units of lattice sites). 
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
  + axes: length of semi-principal axes along x, y, z directions
- \b Cylinder: 
  + origin: origin of cylinder, i.e. one end
  + length: vector to the other cylinder end
  + radius: radius of cylinder

\section Examples

Initializing a single circle or sphere:
\verbatim
<InitCellObjects mode="distance">
	<Arrangement repetitions="1,1,1" displacements="0,0,0">
		<Sphere center="50,50,0" radius="50"/>
	</Arrangement>
</InitCellObjects>
\endverbatim

Initializing 6*6 circles or spheres:
\verbatim
<InitCellObjects mode="distance">
	<Arrangement repetitions="6,6,0" displacements="10,10,0">
		<Sphere center="10,10,0" radius="10"/>
	</Arrangement>
</InitCellObjects>
\endverbatim

Hexagonal tesselation:
choose lattice size (repetitions.x * displacements.x, repetitions.y * displacements.y, 0)
\verbatim
<InitCellObjects mode="distance">
	<Arrangement repetitions="16,14,0" displacements="32,40,0">
		<Sphere radius="30" center="16,16,0"/>
	</Arrangement>
	<Arrangement repetitions="16,14,0" displacements="32,40,0">
		<Sphere radius="30" center="32,40,0"/>
	</Arrangement>
</InitCellObjects>
\endverbatim

Rhombic dodecahedrons using expressions
Choose lattice size (r.x*d.x, r.y*d.y, r.z*d.z)

\verbatim
<Global>
   <ConstantVector symbol="r" name="repetitions" value="5,4,3" />
   <ConstantVector symbol="p" name="position" value="16,16,16" />
   <ConstantVector symbol="d" name="displacements" value="2*p.x, 3*p.y, 3*p.z" />
</Global>
...
<InitCellObjects mode="distance">
	<Arrangement repetitions="r" displacements="d">
		<Sphere radius="21" center="p"/>
	</Arrangement>
	<Arrangement repetitions="r" displacements="d">
		<Sphere radius="21" center="2*p.x, 2.5*p.y, p.z"/>
	</Arrangement>
	<Arrangement repetitions="r" displacements="d">
		<Sphere radius="21" center="2*p.x, 1.5*p.y, 2.5*p.z"/>
	</Arrangement>
	<Arrangement repetitions="r" displacements="d">
		<Sphere radius="21" center="p.x, 3*p.y, 2.5*p.z"/>
	</Arrangement>
</InitCellObjects>
\endverbatim

*/

class InitCellObjects : public Population_Initializer
{
	
public:
	InitCellObjects();
	DECLARE_PLUGIN("InitCellObjects");
	void loadFromXML(const XMLNode, Scope* scope) override;
	vector<CPM::CELL_ID> run(CellType* ct) override;
	
	/// Interface class for Objects to be placed.
	class CellObject {
		public:
			virtual ~CellObject() {};
			virtual string name() const =0;
			virtual VDOUBLE center() const =0;
			virtual void init() = 0;
			virtual double affinity(const VDOUBLE& pos) const =0;
			virtual bool inside(const VDOUBLE& pos) const =0;
			virtual void displace(VDOUBLE distance) =0;
			virtual unique_ptr<CellObject> clone() const =0;
			void setCellID(CPM::CELL_ID id) { cell_id= id; } 
			CPM::CELL_ID cellID() { return cell_id; } 
		private: 
			CPM::CELL_ID cell_id = CPM::NO_CELL;
	};
	
private:
	enum class  Mode { ORDER, DISTANCE };
	PluginParameter2<Mode, XMLNamedValueReader, DefaultValPolicy> mode;
	shared_ptr<const Lattice> lattice;
	
	struct Candidate{
		int index;
		double affinity;
	};

	vector< unique_ptr<CellObject> > cellobjects;
	int setNodes(CellType* ct);
	void arrangeObjectCombinatorial( unique_ptr<CellObject> c_template, vector< unique_ptr<CellObject> >& objectlist, VDOUBLE displacement, VINT repetitions, double random_displacement);
	VDOUBLE distanceToLineSegment(VDOUBLE p, VDOUBLE l1, VDOUBLE l2);
};

#endif //INITSPHERECELL_H
