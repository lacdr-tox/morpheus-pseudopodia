#include "cell.h"
#include "celltype.h"

using namespace SIM;

/// Gives orientation [0,2pi] of semi-minor axis in elliptic approximation
VDOUBLE Cell::getOrientation() const {
	return shape_tracker.current().ellipsoidApprox().axes[0];
}

double Cell::getCellLength() const {
	// get the maximum length (should in fact always be the first in the vector)
	return shape_tracker.current().ellipsoidApprox().lengths.front();
}

VDOUBLE Cell::getMajorAxis() const {
	return shape_tracker.current().ellipsoidApprox().axes.front(); //cell_axis::major];
}

VDOUBLE Cell::getMinorAxis() const {
	if (shape_tracker.current().ellipsoidApprox().axes.size()>1)
		return shape_tracker.current().ellipsoidApprox().axes[1];
	else
		return VDOUBLE();
}

double Cell::getEccentricity() const {
	return shape_tracker.current().ellipsoidApprox().eccentricity;
}


Cell::Cell(CPM::CELL_ID cell_name, CellType* ct)
		: properties(p_properties), id(cell_name), name(to_str(cell_name)), celltype(ct), nodes(), shape_tracker(cell_name,nodes)
{
	for (uint i=0;i< celltype->default_properties.size(); i++) {
		p_properties.push_back(celltype->default_properties[i]->clone());
	}
// 	for (uint i=0;i< celltype->default_membranes.size(); i++) {
// 		p_membranes.push_back(celltype->default_membranes[i]->clone());
// 	}
	track_nodes = true;
	track_shape = true;
	node_sum = VINT(0,0,0);
	centerL = VINT(0,0,0);
	center = VINT(0,0,0);
};

void Cell::init()
{
// 	cout << "initializing cell " << id << endl;
	for (auto prop : p_properties) {
// 		try {
			prop->init(SymbolFocus(id));
// 		}
// 		catch (string e) { throw MorpheusException(e); }
	}
}


Cell::Cell( Cell& other_cell, CellType* ct  )
		: properties(p_properties), id(other_cell.getID()), name(other_cell.name), celltype (ct),nodes(other_cell.nodes), node_sum(other_cell.node_sum),
		centerL(other_cell.centerL), center(other_cell.center), shape_tracker(id, nodes) 
{
	
	for (uint i=0;i< celltype->default_properties.size(); i++) {
		p_properties.push_back(celltype->default_properties[i]->clone());
	}
	
	track_nodes = other_cell.track_nodes;
	track_shape = other_cell.track_shape;
};

Cell::~Cell() {
	// all containers moved to shared_ptr -- auto cleanup
}

void Cell::disableNodeTracking() {
	track_nodes = false; nodes.clear();
}

void Cell::setShapeTracking(bool state)
{
	if (track_shape != state) {
		track_shape = state;
		if (track_shape)
			shape_tracker.reset();
	}
}



void Cell::assignMatchingProperties(const vector< shared_ptr<AbstractProperty> > other_properties){
	// copy all cell properties with matching names & types
	for (uint o_prop=0; o_prop< other_properties.size(); o_prop++) {
		if (other_properties[o_prop]->symbol()[0]=='_') continue; // skip intermediates ...
		for (uint prop=0; prop < p_properties.size(); prop++) {
			if (p_properties[prop]->symbol() == other_properties[o_prop]->symbol() && p_properties[prop]->type() == other_properties[o_prop]->type()) {
				p_properties[prop]->assign(other_properties[o_prop]);
				break;
			}
		}
	}
}

void Cell::loadNodesFromXML(const XMLNode xNode) {
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

void Cell::loadFromXML(const XMLNode xNode) {
	if (!getXMLAttribute(xNode, "name",name)) {
		name = to_str(id);
	}
	// Try loading properties from XMLNode
	for (auto prop : properties) {
		XMLNode xData = xNode.getChildNodeWithAttribute(prop->XMLDataName().c_str(),"symbol-ref",prop->symbol().c_str());
		if (!xData.isEmpty()){
			prop->restoreData(xData);
		}
	}

}

XMLNode Cell::saveToXML() const {
	XMLNode xCNode = XMLNode::createXMLTopNode("Cell");
	xCNode.addAttribute("id",to_str(id).c_str());
	if (to_str(id) != name) {
		xCNode.addAttribute("name",name.c_str());
	}
	
	// save properties to XMLNode
	for (uint prop=0; prop < properties.size(); prop++) {
		xCNode.addChild(properties[prop]->storeData());
	}

 	if (track_nodes) {
 		xCNode.addChild("Center").addText( to_cstr(getCenter()) );
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


void Cell::setUpdate(const CPM::Update& update)
{
	if (track_nodes) {
		if ( ! ( nodes.size() == 1 and update.opRemove() ) ) {
			if (track_shape) shape_tracker.setUpdate(update);
		}
	}
}

const EllipsoidShape& Cell::getEllipsoidShape() const
{
	return currentShape().ellipsoidApprox();
}


const PDE_Layer& Cell::getSphericalApproximation() const
{
	return currentShape().sphericalApprox();
}



void Cell::applyUpdate(const CPM::Update& update)
{
	if (track_nodes) {
		if (update.opAdd()) {
			nodes.insert( update.focusStateAfter().pos );
			node_sum += update.focusStateAfter().pos;
			centerL = VDOUBLE(node_sum) / nodes.size();
			center = SIM::lattice().to_orth(centerL);
		}
		if (update.opRemove()) {
			if ( ! nodes.erase(update.focusStateBefore().pos) ) {
				cerr << "Cell::applyUpdate : Trying to remove a node "<< update.focusStateBefore().pos << " that was not stored! " << endl;
				cerr << CPM::getNode(update.focusStateBefore().pos) << endl;
				cerr << update.focusStateBefore() << " " << celltype->getName() << endl;
				copy(nodes.begin(), nodes.end(), ostream_iterator<VINT>(cout,"|"));
				exit(-1);
			}
			node_sum -= update.focusStateBefore().pos;
			centerL = VDOUBLE(node_sum) / nodes.size();
			center = SIM::lattice().to_orth(centerL);
		}
		if (track_shape) shape_tracker.applyUpdate(update);
	}
}
