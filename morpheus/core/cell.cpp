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
		: properties(p_properties), membranes(p_membranes), id(cell_name), celltype(ct), nodes(), shape_tracker(cell_name,nodes)
{
	for (uint i=0;i< celltype->default_properties.size(); i++) {
		p_properties.push_back(celltype->default_properties[i]->clone());
	}
	for (uint i=0;i< celltype->default_membranes.size(); i++) {
		p_membranes.push_back(celltype->default_membranes[i]->clone());
	}
	track_nodes = true;
	track_shape = true;
	node_sum = VINT(0,0,0);
};

void Cell::init()
{
	for (auto prop : p_properties) {
		try {
			prop->init(celltype->getScope(), SymbolFocus(id));
		}
		catch (string e) { throw MorpheusException(e, prop->saveToXML()); }
	}

	for (auto mem : p_membranes) {
		try {
			mem->init(celltype->getScope(), SymbolFocus(id));
		}
		catch (string e) { throw MorpheusException(e, mem->saveToXML()); }
	}
}


Cell::Cell( Cell& other_cell, CellType* ct  )
		: properties(p_properties), membranes(p_membranes), id(other_cell.getID()), celltype (ct),nodes(other_cell.nodes), node_sum(other_cell.node_sum), shape_tracker(id, nodes)
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
		}
		if (track_shape) shape_tracker.applyUpdate(update);
	}
}
