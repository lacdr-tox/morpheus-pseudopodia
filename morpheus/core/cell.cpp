#include "cell.h"
#include "celltype.h"
#include "membranemapper.h"

using namespace SIM;
using namespace Eigen;


const EllipsoidShape& CellShape::ellipsoidApprox() {
	
	// if cellshape is up-to-date, return this, otherwise recalculate
	if (valid_cache[ELLIPSOID])
		return ellipsoid;
		
	const uint numnodes = nodes.size();
	const Lattice& lattice = SIM::lattice();
	vector<double> I((lattice.getDimensions()-1)*3, 0.0);

	if (lattice.getDimensions()==3){
		for (Cell::Nodes::const_iterator pt = nodes.begin(); pt != nodes.end(); pt++)
		{
			VDOUBLE delta = center - lattice.to_orth(*pt);
			I[LC_XX]+=sqr(delta.y)+sqr(delta.z);
			I[LC_YY]+=sqr(delta.x)+sqr(delta.z);
			I[LC_ZZ]+=sqr(delta.x)+sqr(delta.y);
			I[LC_XY]+=-delta.x*delta.y;
			I[LC_XZ]+=-delta.x*delta.z;
			I[LC_YZ]+=-delta.y*delta.z;		
		}
		
		ellipsoid = calcLengthHelper3D(I,numnodes);
	} 
	else if (lattice.getDimensions()==2){
		for (Cell::Nodes::const_iterator pt = nodes.begin(); pt != nodes.end(); pt++)
		{
			VDOUBLE delta = center - lattice.to_orth(*pt);
			I[LC_XX] += sqr(delta.y);
			I[LC_YY] += sqr(delta.x);
			I[LC_XY] += -delta.x*delta.y;	   
		} 
		ellipsoid = calcLengthHelper2D(I,numnodes);
	} else assert(0);
	
	ellipsoid.center = center;
	ellipsoid.volume = numnodes;
	valid_cache[ELLIPSOID] = true;

	return ellipsoid;
}


const PDE_Layer& Cell::getSphericalApproximation() const
{
	return cell_shape.sphericalApprox();
}

CellShape::CellShape ( const VDOUBLE& cell_center, const set< VINT, less_VINT >& cell_nodes ) : nodes(cell_nodes), center(cell_center)
{
	valid_cache.resize(2);
	valid_cache = false;
	spherical_mapper = NULL;
}

CellShape::~CellShape()
{
	if (spherical_mapper)
		delete spherical_mapper;
}

void CellShape::invalidate()
{
	valid_cache = false;
}



const PDE_Layer& CellShape::sphericalApprox()
{
	if (valid_cache[SPHERIC])
		return spherical_mapper->getData();

	if (!spherical_mapper) {
		spherical_mapper = new MembraneMapper(MembraneMapper::MAP_CONTINUOUS);
	}

	// report radius for all surface nodes
	cout << "creating sphericalApprox around center " << center  << endl;
	spherical_mapper->attachToCenter(center);
	const Lattice& lattice = SIM::lattice();
	
	for (Cell::Nodes::const_iterator pt = nodes.begin(); pt != nodes.end(); pt++)
	{
		if (CPM::isSurface(*pt)) {
			double distance = lattice.orth_distance(center, lattice.to_orth(*pt)).abs();
			spherical_mapper->map(*pt,distance);
		}
	}
	// interpolate and return
	spherical_mapper->fillGaps();
 	//spherical_mapper->flatten();
	valid_cache[SPHERIC] = true;
	return spherical_mapper->getData();
}


EllipsoidShape CellShape::calcLengthHelper3D(const std::vector<double> &I, int N) const
{

	
	EllipsoidShape es;
//  cout << "helper: LengthConstraint::calcLengthHelper3D(const std::vector<double> &I, int N)\n";
	if(N<=1) {
		for (uint i=0; i<3; i++) {
			es.lengths.push_back(N);
			es.axes.push_back(VDOUBLE(0,0,0));
 		}
		return es;
	} // gives nan otherwise
	
	// From of the inertia tensor (principal moments of inertia) we compute the eigenvalues and
	// obtain the cell length by assuming the cell was an ellipsoid
	Matrix3f eigen_m;
	eigen_m << I[LC_XX], I[LC_XY], I[LC_XZ],
	           I[LC_XY], I[LC_YY], I[LC_YZ],
	           I[LC_XZ], I[LC_YZ], I[LC_ZZ];
	SelfAdjointEigenSolver<Matrix3f> eigensolver(eigen_m);
	if (eigensolver.info() != Success) {
		cerr << "Cell::calcLengthHelper3D: Computing eigenvalues was not successfull!" << endl;
	}

	Vector3f eigen_values = eigensolver.eigenvalues();
	Matrix3f EV = eigensolver.eigenvectors();
	Matrix3f Am;
	Am << -1,  1,  1,
	       1, -1,  1,
	       1,  1, -1;
	Array3f axis_lengths = ((Am * eigen_values).array() * (2.5/double(N))).sqrt();
	Vector3i sorted_indices;
	for (uint i=0; i<3; i++) {
		es.lengths.push_back(axis_lengths(i));
		es.axes.push_back(VDOUBLE(EV(0,i),EV(1,i),EV(2,i)).norm());
	}
	// sorting axes by length
	bool done=false;
	while (!done) {
		for (uint i=0;1;i++) {
			if (es.lengths[i] < es.lengths[i+1]) {
				swap(es.lengths[i],es.lengths[i+1]);
				swap(es.axes[i],es.axes[i+1]);
			}
			if (i==2) {
				done=true;
				break;
			}
		}
	} 
	 // axes with correct size 
// 	double f[3];
// 	if (lambda[2]>0) 
// 		 f[2]=sqrt(lambda[2]*5./(A[0]*A[0]+A[1]*A[1])/double(N)); 
// 	else if (lambda[1]>0) 
// 		 f[1]=sqrt(lambda[1]*5./(A[0]*A[0]+A[2]*A[2])/double(N)); 
// 	else 
// 		 f[0]=sqrt(lambda[0]*5./(A[1]*A[1]+A[2]*A[2])/double(N));
// 	 //double l = A[2]*f; 
// 	 
// 	// NOTE: test whether f's are correct
// 	es.lengths.push_back( A[2]*f[2] );
// 	es.lengths.push_back( A[1]*f[1] );
// 	es.lengths.push_back( A[0]*f[0] );

	 return es;
}

EllipsoidShape CellShape::calcLengthHelper2D(const std::vector<double> &I, int N) const
{
	EllipsoidShape es;
//   cout << "helper: LengthConstraint::calcLengthHelper2D(const std::vector<double> &I, int N)\n";
	if (N==1){
		es.lengths.push_back( 1 );
		es.lengths.push_back( 1 );
		es.axes.push_back(VDOUBLE());
		es.axes.push_back(VDOUBLE());
		return es;
		
	}
	
	// long axis
	double lambda_b = 0.5 * (I[LC_XX] + I[LC_YY]) + 0.5 * sqrt( sqr(I[LC_XX] - I[LC_YY]) + 4 * sqr(I[LC_XY]));
    double lambda_a = 0.5 * (I[LC_XX] + I[LC_YY]) - 0.5 * sqrt( sqr(I[LC_XX] - I[LC_YY]) + 4 * sqr(I[LC_XY]));
    
	// TODO: is this the radius (semimajor axis) or diameter (major axis)?
	VDOUBLE major_axis = VDOUBLE(I[LC_XY], lambda_a-I[LC_XX], 0);
	double major_length = 4*sqrt(lambda_b/double(N));

	// short axis
	
	// TODO: is this the radius (semiminor axis) or diameter (minor axis)?
	VDOUBLE minor_axis = VDOUBLE(I[LC_XY], lambda_b-I[LC_XX], 0);
	double minor_length = 4*sqrt(lambda_a/double(N));
	
	double eccentricity = sqrt( 1 - (lambda_a / lambda_b) );
	
    es.axes.push_back( major_axis );
    es.axes.push_back( minor_axis );
	
	es.lengths.push_back( major_length );
	es.lengths.push_back( minor_length );
	es.eccentricity = eccentricity;

	return es;
}

const EllipsoidShape& Cell::getCellShape() const {
	return cell_shape.ellipsoidApprox();
}

/// Gives orientation [0,2pi] of semi-minor axis in elliptic approximation
VDOUBLE Cell::getOrientation(){
	return getMajorAxis();
}

double Cell::getCellLength() const {
	// get the maximum length (should in fact always be the first in the vector)
	return cell_shape.ellipsoidApprox().lengths.front();
}

VDOUBLE Cell::getMajorAxis() const {
	return cell_shape.ellipsoidApprox().axes.front(); //cell_axis::major];
}

VDOUBLE Cell::getMinorAxis() const {
	if (cell_shape.ellipsoidApprox().axes.size()>1)
		return cell_shape.ellipsoidApprox().axes[1];
	else
		return VDOUBLE();
}

double Cell::getEccentricity() const {
	return cell_shape.ellipsoidApprox().eccentricity;
}

// VDOUBLE Cell::getCellVolume(){
// 	ellipsoidApproximation();
// 	return cellshape.volume;
// }
// 
// VDOUBLE Cell::getCellCenter(){
// 	ellipsoidApproximation();
// 	return cellshape.center;
// }


// VDOUBLE long_cell_axis(const Cell::Nodes& points) {
// 	if (points.size() < 2) return VDOUBLE();
//
// 	VDOUBLE center = accumulate(points.begin(),points.end(),VINT(0,0,0));
// 	center = center / double(points.size());
//
// 	double Ixx=0,Iyy=0,Ixy=0;
// 	for (Cell::Nodes::const_iterator pt = points.begin(); pt != points.end(); pt++)
// 	{
// 		VDOUBLE delta = center - VDOUBLE(*pt);
// 		Ixx += sqr(delta.y);
// 		Iyy += sqr(delta.x);
// 		Ixy += -delta.x*delta.y; // was wrong (without minus)
// 	}
// 	double lambda_a=0.5 * (Ixx + Iyy) + 0.5 * sqrt( sqr(Ixx - Iyy) + 4 * sqr(Ixy)); // was wrong
//
// 	VDOUBLE main_axis = VDOUBLE(Ixy, lambda_a-Ixx, 0);
// 	//main_axis.norm();
// 	return main_axis;
// }

// double long_cell_axis2(const Cell::Nodes& nodes) {
// 	if (nodes.size() < 2) return 0.0;
//
// 	VDOUBLE center = accumulate(nodes.begin(),nodes.end(),VINT(0,0,0));
// 	center = center / double(nodes.size());
//
// 	double Ixx=0,Iyy=0,Ixy=0;
// 	for (Cell::Nodes::const_iterator pt = nodes.begin(); pt != nodes.end(); pt++)
// 	{
// 		VDOUBLE delta = center - VDOUBLE(*pt);
// 		Ixx += sqr(delta.y);
// 		Iyy += sqr(delta.x);
// 		Ixy += -delta.x*delta.y;
// 	}
// 	double lambda_a=0.5 * (Ixx + Iyy) + 0.5 * sqrt( sqr(Ixx - Iyy) + 4 * sqr(Ixy));
// 	double length =	4*sqrt(lambda_a/double(nodes.size()));
// 	return length;
// }

const Cell::Nodes& Cell::getSurface() const {

// 	cout << "getSurface: " << surface.size() << endl;
	// if surface might have changed, compute new surface
	if( SIM::getTime() != surface_timestamp ){
		surface.clear();

		Nodes::const_iterator i = nodes.begin();
		
		for(i = nodes.begin(); i!= nodes.end(); i++){
			if( CPM::isSurface(*i) ){
				surface.insert(surface.end(), *i );	
			}
		}
		surface_timestamp= SIM::getTime();
		
	}
	// otherwise, return precomputed surface
	return surface;
}

Cell::Cell(CPM::CELL_ID cell_name, CellType* ct)
		: properties(p_properties), membranes(p_membranes), id(cell_name), celltype(ct), cell_shape(orth_center, nodes)
{
	for (uint i=0;i< celltype->default_properties.size(); i++) {
		p_properties.push_back(celltype->default_properties[i]->clone());
	}
	for (uint i=0;i< celltype->default_membranes.size(); i++) {
		p_membranes.push_back(celltype->default_membranes[i]->clone());
	}
	track_nodes = true;
	track_surface = true;
	interface_length=0;
	neighbors2length = CPMShape::BoundaryLengthScaling(CPM::getBoundaryNeighborhood());
	surface_timestamp = -10000;
};

void Cell::init()
{
	for (auto prop : p_properties) {
		prop->init(celltype->getScope(), SymbolFocus(id));
	}
	for (auto mem : p_membranes) {
		mem->init(celltype->getScope(), SymbolFocus(id));
	}
}


Cell::Cell( Cell& other_cell, CellType* ct  )
		: properties(p_properties), membranes(p_membranes), id(other_cell.getID()), celltype (ct), cell_shape(orth_center, nodes)
{
	
	for (uint i=0;i< celltype->default_properties.size(); i++) {
		p_properties.push_back(celltype->default_properties[i]->clone());
	}
	for (uint i=0;i< celltype->default_membranes.size(); i++) {
		p_membranes.push_back(celltype->default_membranes[i]->clone());
	}
	assignMatchingProperties(other_cell.properties);
	assignMatchingMembranes(other_cell.membranes);
	
	track_nodes = other_cell.track_nodes;
	track_surface = other_cell.track_nodes;
	surface_timestamp = -10000;
	nodes = other_cell.nodes;                     
	surface = other_cell.surface;       
	interfaces = other_cell.interfaces;
	interface_length = other_cell.interface_length;
	neighbors2length = other_cell.neighbors2length;
	accumulated_nodes = other_cell.accumulated_nodes;
	orth_center = other_cell.orth_center;     
};

Cell::~Cell() {
	// all containers moved to shared_ptr -- auto cleanup
}

void Cell::disableNodeTracking() {
	track_nodes = false; nodes.clear();
	track_surface=false; surface.clear();
}

// void Cell::setSurfaceTracking(bool state) {
// 	if (track_surface) {
// 		if (! state) {
// 			surface.clear();
// 			interfaces.clear();
// 			track_surface = false;
// 		}
// 	}
// 	else if (state) {
// 		if (! isNodeTracking()) {
// 			cerr << " Cell::setSurfaceTracking: cannot enable Boundary Tracking without Node Tracking." << endl;
// 			exit(-1);
// 		}
// 		track_surface = true;
// 		if (nodes.size()) {
// 			static vector<VINT> neighborhood = CPM::getBoundaryNeighborhood();
// 			// get the interaction neighborhood, and parse all nodes to initialize the tracker
// 			for (Nodes::const_iterator node  = nodes.begin(); node != nodes.end(); ++node ) {
// 				bool node_is_boundary=false;
// 				for (vector<VINT>::const_iterator offset  = neighborhood.begin(); offset != neighborhood.end(); ++offset ) {
// 					const CPM::STATE& neighbor = CPM::getNode( (*node) + (*offset));
// 					if (neighbor.cell_id != id) {
// 						node_is_boundary = true;
// 						interfaces[neighbor.cell_id]++;
// 					}
// 				}
// 				if (node_is_boundary) surface.insert(*node);
// 			}
// 		}
// 	}
// };

void Cell::assignMatchingProperties(const vector< shared_ptr<AbstractProperty> > other_properties){
	// copy all cell properties with matching names & types
	for (uint o_prop=0; o_prop< other_properties.size(); o_prop++) {
		if (other_properties[o_prop]->getSymbol()[0]=='_') continue; // skip intermediates ...
		for (uint prop=0; prop < p_properties.size(); prop++) {
			if (p_properties[prop]->getSymbol() == other_properties[o_prop]->getSymbol() && p_properties[prop]->getTypeName() == other_properties[o_prop]->getTypeName()) {
				p_properties[prop] = other_properties[o_prop]->clone();
				break;
			}
		}
	}
}

void Cell::assignMatchingMembranes(const vector< shared_ptr<PDE_Layer> > other_membranes) {
	for (uint i=0;i< p_membranes.size(); i++) {
		uint j=0;
		for (; j<other_membranes.size(); j++) {
			if (other_membranes[j]->getSymbol() == p_membranes[i]->getSymbol() ) {
				p_membranes[i] = other_membranes[j]->clone();
				break;
			}
		}
	}
}

void Cell::loadFromXML(const XMLNode xNode) {

	
	// load matching properties from XMLNode
	vector<XMLNode> property_nodes;
	for (uint p=0; p<properties.size(); p++) {
		properties[p]->restoreData(xNode);
	}
	
//	TODO: load membraneProperties from XML
	for (uint mem=0; mem<xNode.nChildNode("MembranePropertyData"); mem++) {
		XMLNode xMembraneProperty = xNode.getChildNode("MembranePropertyData",mem);
		string symbol; getXMLAttribute(xMembraneProperty, "symbol-ref", symbol);

		uint p=0;
		for (; p<membranes.size(); p++) {
			if (membranes[p]->getSymbol() == symbol) {
				//string filename = membranes[mem]->getName() + "_" + to_string(id)  + "_" + SIM::getTimeName() + ".dat";
				membranes[p]->restoreData(xMembraneProperty);
// 				string filename; getXMLAttribute(xMembraneProperty, "filename", filename);
// 				cout << "Loading MembranePropertyData '" << symbol << "' from file '" << filename << "'. Sum = " << membranes[p]->sum() << endl;
				break;
			}
		}
		if (p==membranes.size()) {
			cerr << "Cell::loadFromXML: Unable to load data for MembranePropertyData " << symbol 
			     << " cause it's not defined for this celltype (" << celltype->getName() << ")"<<endl;
			exit(-1);
		}
	}
	
	string snodes;
	if ( getXMLAttribute(xNode,"Nodes/text",snodes,false) ) {
		stringstream ssnodes(snodes);
		char sep; VINT val;

		VINT position;
		while (1) {
			ssnodes >> position;
			if (ssnodes.fail()) break;
			if ( ! CPM::setNode(position, id) ) {
				cout << "Cell::loadFromXML  unable to put cell [" << id << "] at " << position << endl; break;
			}
			ssnodes >> sep;
			if ( sep != ';' and sep != ',') break;
		}
	}
	else // no nodes specified
	{
		cout << "Cell " << id << " already has " << CPM::getCell(id).getNodes().size() << " nodes." << endl;
	}
}

XMLNode Cell::saveToXML() const {
	XMLNode xCNode = XMLNode::createXMLTopNode("Cell");
	xCNode.addAttribute("name",to_cstr(id));
	
	// save properties to XMLNode
	for (uint prop=0; prop < properties.size(); prop++) {
		xCNode.addChild(properties[prop]->storeData());
	}

	for (uint mem=0; mem < membranes.size(); mem++) {
		
// 		string path_cwd;
// 		char *path = NULL;
// 		path = getcwd(NULL, 0); // or _getcwd
// 		if ( path != NULL){
// 			path_cwd = string(path);
// 			//cout << path_cwd << endl;
// 		}
// 		string filename =  string(path) + "/" + membranes[mem]->getName() + "_" + to_str(id)  + "_" + SIM::getTimeName() + ".dat";
		string filename = membranes[mem]->getName() + "_" + to_str(id)  + "_" + SIM::getTimeName() + ".dat";

		XMLNode node = membranes[mem]->storeData(filename);
		node.updateName("MembranePropertyData");
		node.addAttribute("symbol-ref",membranes[mem]->getSymbol().c_str());
		xCNode.addChild(node);
	}
 	if (track_nodes) {
 		xCNode.addChild("Center").addText( to_cstr(getCenter(),6) );
 		ostringstream node_data;
 		for (Nodes::const_iterator inode = nodes.begin(); inode != nodes.end(); inode++ )
 		{
 			if ( inode != nodes.begin() ) node_data << ";";
 			node_data << *inode;
 		}
 		xCNode.addChild("Nodes").addText(node_data.str().c_str());
 	}
	return xCNode;
}



VDOUBLE Cell::getCenter() const {
// 	if( SIM::getLattice()->getDimensions() == 2 &&
// 		(SIM::getLattice()->get_boundary_type( Boundary::mx ) == Boundary::periodic ||
// 		SIM::getLattice()->get_boundary_type( Boundary::my ) == Boundary::periodic )){
// 		return getCenterOfMassPeriodic();
// 	}
	return orth_center;
}

VINT Cell::getCenterL() const {
// 	if( SIM::getLattice()->getDimensions() == 2  &&
// 		(SIM::getLattice()->get_boundary_type( Boundary::mx ) == Boundary::periodic ||
// 		SIM::getLattice()->get_boundary_type( Boundary::my ) == Boundary::periodic )){
// 		
// 		return VINT( getCenterOfMassPeriodic() );
// 	}
	return lattice_center;
}

VDOUBLE Cell::getCenterOfMassPeriodic() const {
/*
 * Bai, Linge; Breen, David (2008). "Calculating Center of Mass in an Unbounded 2D Environment". Journal of Graphics, GPU, and Game Tools 13 (4): 53–60. doi:10.1080/2151237X.2008.10129266.
	https://www.cs.drexel.edu/~david/Papers/Bai_JGT.pdf
*/
	assert( SIM::getLattice()->getDimensions() == 2 );
	assert( (SIM::getLattice()->get_boundary_type( Boundary::mx ) == Boundary::periodic || SIM::getLattice()->get_boundary_type( Boundary::my ) == Boundary::periodic ));
	
	double r_i = SIM::lattice().size().x / (2.0*M_PI);
	double phi_i_0 = 2.0*M_PI / SIM::lattice().size().x;
	double r_j = SIM::lattice().size().y / (2.0*M_PI);
	double phi_j_0 = 2.0*M_PI / SIM::lattice().size().y;
	VDOUBLE X_Ti(0.0, 0.0, 0.0);
	VDOUBLE X_Tj(0.0, 0.0, 0.0);
	int count=0;
	for (Nodes::const_iterator inode = nodes.begin(); inode != nodes.end(); inode++ )
	{
		double phi_i = inode->x * phi_i_0;
		double phi_j = inode->y * phi_j_0;
		VDOUBLE Xi( r_i*cos(phi_i), inode->y, r_i*sin(phi_i) ); 
		VDOUBLE Xj( inode->x, r_j*cos(phi_j), r_j*sin(phi_j) ); 
		
		X_Ti += Xi;
		X_Tj += Xj;
		count++;
	}	
	X_Ti.x /= double(count);
	X_Ti.y /= double(count);
	X_Ti.z /= double(count);
	X_Tj.x /= double(count);
	X_Tj.y /= double(count);
	X_Tj.z /= double(count);

	double phi_i = atan2(-X_Ti.z, -X_Ti.x)+M_PI;
	double phi_j = atan2(-X_Tj.z, -X_Tj.y)+M_PI;
	double i_p =  phi_i * SIM::lattice().size().x / (2.0*M_PI);
	double j_p =  phi_j * SIM::lattice().size().y / (2.0*M_PI);
	VDOUBLE center( i_p, j_p, 0.0 );
	return center;
}

std::map< CPM::CELL_ID, double > Cell::getInterfaceLengths() const
{
	 map<CPM::CELL_ID,double> lengths(interfaces.begin(), interfaces.end() );
	 
	 map <CPM::CELL_ID, double >::iterator it = lengths.begin();
	 for (; it != lengths.end(); it++) {
		 it->second /= neighbors2length;
	 }
	 return lengths;
}


void Cell::resetUpdatedInterfaces() {
	map <CPM::CELL_ID, uint >::iterator ui, i;
	bool brute_force_copy = false;
	if (updated_interfaces.size() == interfaces.size()) {
		for (ui = updated_interfaces.begin(), i=interfaces.begin(); i != interfaces.end(); ++ui, ++i) {
			if (ui->first == i->first) {
				ui->second = i->second;
// 				assert(ui->second);
			}
			else {
				// a new neighbor appeared
				brute_force_copy = true;
				break;
			}
		}
	}
	else 
		brute_force_copy = true;
	
	if (brute_force_copy)
		updated_interfaces = interfaces;
	updated_interface_length = interface_length;
}

void Cell::setUpdate(const CPM::Update& update)
{
	if ( ! ( nodes.size() == 1 and update.opRemove() ) ) {
		if (track_nodes) {
			if (update.opAdd()) {
				updated_center = SIM::lattice() . to_orth(  (VDOUBLE)(accumulated_nodes + update.focusStateAfter().pos) / ( nodes.size() + 1 ) );
				updated_lattice_center  =( ( accumulated_nodes + update.focusStateAfter().pos) / ( nodes.size() + 1 ) );
			}
			else if (update.opRemove()) {
				updated_center = SIM::lattice() . to_orth(  (VDOUBLE)(accumulated_nodes - update.focusStateBefore().pos) / ( nodes.size() - 1 ) );
				updated_lattice_center  =( ( accumulated_nodes - update.focusStateBefore().pos) / ( nodes.size() - 1 ) );
			}
		}

		if (track_surface) {
			
			resetUpdatedInterfaces();
			const vector<StatisticalLatticeStencil::STATS>& neighbor_stats = update.boundaryStencil()->getStatistics();
			int interface_diff=0;
			
			if (update.opAdd()) {
				for ( auto neighbor = neighbor_stats.begin(); neighbor != neighbor_stats.end(); neighbor++ ) {
					if (neighbor->cell == id) {
						updated_interfaces[update.focusStateBefore().cell_id] -= neighbor->count;
						interface_diff -= neighbor->count;
					}
					else  {
						updated_interfaces[neighbor->cell] += neighbor->count;
						interface_diff += neighbor->count;
					}
				}
			}
			else if (update.opRemove()) {
				for ( auto neighbor = neighbor_stats.begin(); neighbor != neighbor_stats.end(); neighbor++ ) {
					if (neighbor->cell == id ) {
						updated_interfaces[ update.focusStateAfter().cell_id ] += neighbor->count;
						interface_diff += neighbor->count;
					}
					else {
						updated_interfaces[ neighbor->cell ] -= neighbor->count;
						interface_diff -= neighbor->count;
					}
				}
			}
			else if (update.opNeighborhood()) {
				// TODO This has to be integrated to allow tracking of boundary changes of neighbor cells
				/*
				const auto& stats = update.boundaryStencil().getStatistics();
				int count = find_if(stats.begin(), stats.end(),[](const StatisticalLatticeStencil::STATS& a) { return a.cell == id ;} )->count;
				if ( (interfaces[update.focusStateBefore().cell_id] -= count) == 0) {
					interfaces.erase(update.focusStateBefore().cell_id);
				}
				interfaces[update.focusStateAfter().cell_id]+=count;
				*/
			}
			
			updated_interface_length += double(interface_diff) / neighbors2length;
		}
	} else {
		updated_center = VDOUBLE(0.0,0.0,0.0);
		updated_lattice_center = VINT(0.0,0.0,0.0);
		updated_interfaces.clear();
		updated_surface.clear();
	}
// 	if ( int(SIM::getTime()) % 100 ==0) {
// 		for ( map <CELL_ID, uint >::const_iterator i = updated_interfaces.begin(); i != updated_interfaces.end(); i++) {
// 			cout << i->first << " => " << i->second << "  ";
// 		}
// 		cout << endl;
// 	}
}

void Cell::applyUpdate(const CPM::Update& update)
{
	if (track_nodes) {
		if (update.opAdd()) {
			nodes.insert( update.focusStateAfter().pos );
			accumulated_nodes += update.focusStateAfter().pos;
			orth_center = updated_center;
			lattice_center = updated_lattice_center;
		}
		if (update.opRemove()) {
			if ( ! nodes.erase(update.focusStateBefore().pos) ) {
				cerr << "Cell::applyUpdate : Trying to remove a node "<< update.focusStateBefore().pos << " that was not stored! " << endl;
				cerr << CPM::getNode(update.focusStateBefore().pos) << endl;
				cerr << update.focusStateBefore() << " " << celltype->getName() << endl;
				copy(nodes.begin(), nodes.end(), ostream_iterator<VINT>(cout,"|"));
				exit(-1);
			}
			accumulated_nodes -= update.focusStateBefore().pos;
			orth_center = updated_center;
			lattice_center = updated_lattice_center;
		}
		cell_shape.invalidate();
	}
	if (track_surface) {
		if (update.opNeighborhood()) {
			// we are notified of an update that we are not directly involved in,
			// i.e. the cell nodes will not change but the interfaces
			const auto& stats = update.boundaryStencil()->getStatistics();
			int count = find_if(stats.begin(), stats.end(),[&](const StatisticalLatticeStencil::STATS& a) { return a.cell == id ;} )->count;
			if ( (interfaces[update.focusStateBefore().cell_id] -= count) == 0) {
				interfaces.erase(update.focusStateBefore().cell_id);
			}
			interfaces[update.focusStateAfter().cell_id]+=count;
		}
		else {
			map <CPM::CELL_ID, uint >::iterator ui, i;
			bool brute_force_copy = false;
			if (updated_interfaces.size() == interfaces.size())
				for (ui = updated_interfaces.begin(), i=interfaces.begin(); ui != updated_interfaces.end(); ++ui) {
					if (ui->first == i->first) {
						if ( ! ui->second ) {
							interfaces.erase(i++);
						} else {
							i->second = ui->second;
							++i;
						}
					}
					else {
						brute_force_copy = true;
						break;
					}
				}
			else brute_force_copy = true;
			if (brute_force_copy) { /*interfaces = updated_interfaces;*/
				interfaces.clear();
				map <CPM::CELL_ID, uint >::iterator i, ui;
				i = interfaces.begin();
				for (ui = updated_interfaces.begin();  ui != updated_interfaces.end(); ui++) {
					if (ui->second != 0) {
						i=interfaces.insert(i,*ui);
					}
				}
			}
			
			interface_length = updated_interface_length;
		}
	}
}

// void Cell::applyNeighborhoodUpadate(const CPM::UPDATE& update, uint count) {
// 	if (track_surface) {
// 		// we are notified of an update that we are not directly involved in,
// 		// i.e. the cell nodes will not change but the interfaces
// 		if ( (interfaces[update.remove_state.cell_id] -= count) == 0) {
// 			interfaces.erase(update.remove_state.cell_id);
// 		}
// 		interfaces[update.add_state.cell_id]+=count;
// 	}
// }
// 


// const Cell::Nodes& Cell::getSurface() const{
// 	if ( ! isSurfaceTracking()) {
// 		static vector<VINT> neighbors = SIM::getCPMBoundaryNeighborhood();
// // 		static int nbs = neighbors.size();
// 		static Cell::Nodes temp_surface;
// 		temp_surface.clear();
// 		Cell::Nodes::const_iterator i;
// 		for (i=nodes.begin(); i != nodes.end(); ++i) {
// 			// if neighbors are not part of the cell, then i is boundary
// 			if( SIM::isCPMBoundary(*i) ){
// 				temp_surface.insert(*i);
// 			}
// 		}
// 		return temp_surface;
// 	}
// 	return surface;
// }

