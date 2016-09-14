#ifndef CPMSHAPE_H
#define CPMSHAPE_H

#include "vec.h"
#include "lattice.h"
#include "cpm_layer.h"


/** 
 * \brief CPMShape keeps the basic logics of a CPMShape 
 * 
 * 
 * */

struct EllipsoidShape{
	vector< VDOUBLE > axes;
	vector< double> lengths;
	double eccentricity;
	VDOUBLE center;
	uint volume;
	double timestamp;
};

class CPMShape
{
public:
	typedef set<VINT,less_VINT> Nodes;
	enum class BoundaryScalingMode {Magno, NeigborNumber, None};
	static double BoundaryLengthScaling(const Neighborhood& neighborhood);
	static Neighborhood boundaryNeighborhood;
	static BoundaryScalingMode scalingMode;
};




/** CPM shape tracker atop of an external node storage **/

// class CPMShapeTracker {
// public:
// 	CPMShapeTracker(const vector<VINT>& nodes );
// 	
// 	// modifiers
// 	void addNode(const VINT& node);
// 	void removeNode(const VINT& node);
// 	void moveNode(const VINT& node, const VINT& new_node);
// 	void copyUpdate(const CPMShapeTracker& other);
// 	
// 	// accessors
// 	const set<VINT> nodes();
// 	/// center in orthogonal coordinates
// 	const VDOUBLE center() const;
// 	/// center in lattice coordinates
// 	const VDOUBLE centerLatt() const;
// 	/// perimeter length with respect to the boundary neighborhood selected
// 	double perimeter() const;
// 	const map<CPM::CELL_ID, double>& InterfaceLengths() const;
// 	
// 	const EllipsoidShape& ellipsoid() const;
// 	
// 	/// Surface nodes. All nodes at the boundary with respect to a neighborhood of
// 	/// distance sqrt(2)
// 	const set<VINT>& surface() const;  // boundary nodes
// 	
// 	
// 	 	
// private:
// 	mutable EllipsoidShape elipsoid;
// 	bool valid_elipsoid;
// };

#endif // CPMSHAPE_H
