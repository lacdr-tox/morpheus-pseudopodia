#ifndef CPM_SHAPE_TRACKER_H
#define CPM_SHAPE_TRACKER_H

#include "cpm_layer.h"
#include "cpm_shape.h"
#include "cell_update.h"

class MembraneMapper;
class PDE_Layer;

/**  @brief Adaptice shape tracker to attach to  cpm cell and it's nodes container
 * 
 * This is a basic building block to perform all the computations in a adaptive manner,
 * i.e. either on the fly or by tracking depending on the request frequency
 */

class AdaptiveCPMShapeTracker {

public:
	/**  Construcor to attach to @cell and it's nodes container @cell_nodes
	 *   Spatial information can be obtained from cell IDs stored in the cell_layer
	 */
	
	AdaptiveCPMShapeTracker(CPM::CELL_ID cell, const CPMShape::Nodes& cell_nodes);
	
	/// Cell nodes accessors
// 	const CPMShape::Nodes& nodes() const { return _nodes; }; // For performance reasons, the pending updated shape does not have an updated notes container yet. Thus, no accessor here.
	/// Cell size
	const int size() const { return node_count; };
	/// Cell center in orthogonal coordinates
	VDOUBLE center() const { return lattice.to_orth(VDOUBLE(node_sum)/double(node_count)); }
	/// Cell center in lattice coordinates
	VDOUBLE centerLatt() const { return VDOUBLE(node_sum)/double(node_count); };
	/// Cell surface as estimated through boundary neighborhood and boundary scaling
	double surface() const { return _interface_length / boundary_scaling; };
	/// Cell surface nodes -- all nodes that have contact via edge or corner to foreign occupied nodes
	const CPMShape::Nodes& surfaceNodes() const;
	/// Interface length to other cells / entities
	const map<CPM::CELL_ID,double>&  interfaces() const;
	/// Ellipsoid approximation of the cell
	const EllipsoidShape& ellipsoidApprox() const;
	/// Approcimation of the cell by a deformed sphere. 
	/// The @ret PDE_Layer represents the radii distribution projected on a sphere.
	const PDE_Layer& sphericalApprox() const;
	/// Reset the tracker
	void reset();
	/// Reset the tracker to a former state stored in @other
	void reset(AdaptiveCPMShapeTracker* other);
		/// Apply the @update
	void apply(const CPM::Update& update);
	/// Apply the @update, which may already be cached in @other
	void apply(const CPM::Update& update, AdaptiveCPMShapeTracker* other);
	/// Add the node in the @update
	static EllipsoidShape computeEllipsoid3D(const valarray<double> &I, int N);
	/// Compute a 2D Ellipsoid approximation fromr the moments @I, number of points @N
	static EllipsoidShape computeEllipsoid2D(const valarray<double> &I, int N);

	
	
private:
	// CPM Shape Tracker assembles two trackers into a fast tracking structure of current and updated cpm shape;
	
// 	void updateInterfaces();
	
	void initNodes() const;
	
	void addNode(const CPM::Update& update);
	/// Remove the node in the @update
	void removeNode(const CPM::Update& update);
	/// The neighbor node in the @update has an update
	void neighborNode(const CPM::Update& update);
	/// Compute a 3D Ellipsoid approximation fromr the moments @I, number of points @N
	void initInterfaces() const;
	
	void initSurfaceNodes() const;
	
	void initEllipsoidShape() const;
	
	/// orthoganal position @pos and direction @dir +1 means add the node at @pos and -1 remove the node at @pos
	void trackEllipse(const VINT pos, int dir);
	
	// reference to the cell's node tracking.
// 	shared_ptr<const CPM::LAYER> cell_layer;
	const Lattice& lattice;
	double boundary_scaling;
	
	const CPMShape::Nodes& _nodes;
	const CPM::CELL_ID cell_id;
	long int n_updates;
	mutable int pending_update_op;
	mutable VINT pending_update_node;
	
	
	struct Trackers { bool nodes; bool surface; bool interfaces; bool elliptic_approx; bool spherical_approx; };
	mutable Trackers tracking;
	mutable int node_count;
	mutable VINT node_sum;
		
	mutable CPMShape::Nodes surface_nodes;
	mutable int last_surface_update;
	
	
	mutable double _interface_length;
	mutable map<CPM::CELL_ID, int> _interfaces;
	mutable int last_interface_update;
	mutable bool scaled_interfaces_valid;
	mutable map<CPM::CELL_ID, double> scaled_interfaces;
	
	mutable EllipsoidShape ellipsoid_approx;
	mutable valarray<double> Ell_I;
	mutable long int last_ellipsoid_update;
	mutable long int last_ellipsoid_init;
	static const int Ell_XX=0;
	static const int Ell_XY=1;
	static const int Ell_YY=2;
	static const int Ell_XZ=3;
	static const int Ell_YZ=4;
	static const int Ell_ZZ=5;
	
	mutable MembraneMapper* spherical_mapper;
	mutable long int last_spherical_update;
};



class CPMShapeTracker {
public:
	CPMShapeTracker(CPM::CELL_ID cell_id, const CPMShape::Nodes& cell_nodes);
	const AdaptiveCPMShapeTracker& updated() const { return updated_shape; }
	const AdaptiveCPMShapeTracker& current() const { return current_shape; }
	
	void setUpdate(const CPM::Update& update);
	void applyUpdate(const CPM::Update& update);
	
	void reset();
	
private:
	AdaptiveCPMShapeTracker updated_shape, current_shape;
	bool updated_is_current;
};


#endif // CPM_SHAPE_TRACKER_H