#include "cpm_shape.h"
// #include <map>

CPMShape::BoundaryScalingMode CPMShape::scalingMode = BoundaryScalingMode::Magno;
Neighborhood CPMShape::boundaryNeighborhood = Neighborhood();


double CPMShape::BoundaryLengthScaling(const Neighborhood& neighborhood)
{
	map<Lattice::Structure, vector<double> > magno_scaling_by_order =
	{ { Lattice::Structure::linear,    {0,1,2,3,4} },
	  { Lattice::Structure::square,    {0,1,3,5,11,15,18,26,36} },
	  { Lattice::Structure::hexagonal, {0,2.2,6,10.4,22.1,28.65} },
	  { Lattice::Structure::cubic,     {0,1,5,9,11,23,39,47,70} }
	};
	
	double scaling = 1;
// 	BoundaryScalingMode mode = BoundaryScalingMode::Magno;
	switch (scalingMode) {
		case BoundaryScalingMode::Magno : 
			scaling = magno_scaling_by_order[neighborhood.lattice().structure][neighborhood.order()];
			break;
		case BoundaryScalingMode::NeigborNumber :
			scaling = neighborhood.size();
			break;
		case BoundaryScalingMode::None:
			scaling = 1.0;
			break;
	}
	return scaling;
}