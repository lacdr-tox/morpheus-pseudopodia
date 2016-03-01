#include "rod_mechanics_energy.h"

REGISTER_PLUGIN(Rod_Mechanics);

Rod_Mechanics::Rod_Mechanics() : 
	stiffness_model(tube), 
	axial_stiffness(0.0),
	bending_stiffness(0.0),
	bending_parameter(1.0),
	interface_length_factor(1.0) {};


void Rod_Mechanics::loadFromXML(const XMLNode xNode) {
	Plugin::loadFromXML(xNode);
	// now you may load your params
	string stiffness_model_string;
	if (getXMLAttribute(xNode,"Stiffness/model",stiffness_model_string)) {
		map<string, StiffnessModel> model_map;
		model_map["solid"] = solid;
		model_map["tube"] = tube;
		model_map["exponential"] = exponential;

		map<string, StiffnessModel>::iterator iter = model_map.find(lower_case(stiffness_model_string));
		if ( iter!= model_map.end()) stiffness_model = iter->second;
		else  cout << "unknown stiffness model, using default!\n";
	}

// 	do_reversal = (getXMLAttribute(xNode,"reversal", reversal_time));
// 	getXMLAttribute(xNode, "TargetVolume/symbol-ref", volume_symbol_name);
	getXMLAttribute(xNode, "Stiffness/axial", axial_stiffness);
	getXMLAttribute(xNode, "Stiffness/bending", bending_stiffness);
// 	getXMLAttribute(xNode, "Bending/exponent", bending_parameter);
	getXMLAttribute(xNode, "Orientation/symbol-ref", orientation_symbol_name);
	getXMLAttribute(xNode, "Reversal/symbol-ref", reversed_symbol_name);
	getXMLAttribute(xNode,"InterfaceConstraint/strength", interface_strength);
	getXMLAttribute(xNode,"InterfaceConstraint/length", interface_length_factor);
}

void Rod_Mechanics::init() {
	
	celltype = dynamic_cast<SuperCT*>(SIM::getScope()->getCellType());
	if ( ! celltype ) {
		cerr << XMLName()+" can only be applied on a segmented celltype" << endl;
		exit(-1);
	}
	orientation = SIM::findRWSymbol<VDOUBLE>(orientation_symbol_name);
	if ( ! reversed_symbol_name.empty() )
		reversed = celltype->findCellProperty<double>(reversed_symbol_name);
// 	segment_volume = SIM::findSymbol<double>(volume_symbol_name);
	lattice_dimensions = SIM::getLattice()->getDimensions();
	
	rod_energy = celltype->addProperty<vector<double> >("_rod_energy","Mechanical energy of the rod");
}

bool Rod_Mechanics::update_check( CPM::CELL_ID cell_id, const CPM::UPDATE& update, CPM::UPDATE_TODO todo)
{
	const SuperCell& cell = static_cast<const SuperCell& >(CPM::getCell(cell_id));
// 	if (todo == CPM::ADD_AND_REMOVE) {
// 		uint first_updated_segment = cell.getSubCellPosition(update.add_state.cell_id);
// 		uint last_updated_segment = cell.getSubCellPosition(update.remove_state.cell_id);
// 		if ( last_updated_segment - first_updated_segment == 1 ) {
// 			const map <, uint >& updated_interfaces = CPM::getCell(update.add_state.cell_id).getUpdatedInterfaces();
// 			if ( updated_interfaces.find(update.remove_state.cell_id) == updated_interfaces.end()) return false;
// 		}
// 	}
// 	else {

		if (todo & CPM::ADD) {
			uint segment_id = CPM::getCellIndex(update.add_state.cell_id).sub_cell_id;
			const map <CPM::CELL_ID, uint >& interfaces = CPM::getCell(update.add_state.cell_id).getInterfaces();
			const map <CPM::CELL_ID, uint >& updated_interfaces = CPM::getCell(update.add_state.cell_id).getUpdatedInterfaces();
			const vector<CPM::CELL_ID>& segments = cell.getSubCells();
		
			if (segment_id>0) {
				if (updated_interfaces.find(segments[segment_id-1]) == updated_interfaces.end() ) {
					if (interfaces.find(segments[segment_id-1]) == interfaces.end())
						cout << "SuperCell " << cell_id <<", SubCell " << segment_id << "(" <<  segments[segment_id] << "): Interface to segment " << segment_id-1 << "(" << segments[segment_id-1] << ") missing !" << endl;
					else
						cout << "SuperCell " << cell_id <<", SubCell " << segment_id << "(" <<  segments[segment_id] << "): " <<  interfaces.find(segments[segment_id-1])->second  << "  interfaces to  " << segment_id-1 << "(" << segments[segment_id-1] << ") vanish! " << endl; 
					return false;
				}
			}
			if (segment_id+1 < segments.size()) {
				if (updated_interfaces.find(segments[segment_id+1]) == updated_interfaces.end() ) {
					if (interfaces.find(segments[segment_id+1]) == interfaces.end())
						cout << "SuperCell " << cell_id <<", SubCell " << segment_id << "(" <<  segments[segment_id] << "): Interface to segment " << segment_id+1 << "(" << segments[segment_id+1] << ") missing !" << endl;
					else
						cout << "SuperCell " << cell_id <<", SubCell " << segment_id << "(" <<  segments[segment_id] << "): " <<  interfaces.find(segments[segment_id+1])->second  << "  interfaces to  " << segment_id+1 << "(" << segments[segment_id+1] << ") vanish! " << endl; 
					return false;
				}
			}
		}
		if (todo & CPM::REMOVE) {
			uint segment_id = CPM::getCellIndex(update.remove_state.cell_id).sub_cell_id;
			const map <CPM::CELL_ID, uint >& interfaces = CPM::getCell(update.remove_state.cell_id).getInterfaces();
			const map <CPM::CELL_ID, uint >& updated_interfaces = CPM::getCell(update.remove_state.cell_id).getUpdatedInterfaces();
			const vector<CPM::CELL_ID>& segments = cell.getSubCells();
		
			if (segment_id>0) {
				if (updated_interfaces.find(segments[segment_id-1]) == updated_interfaces.end()) {
					if (interfaces.find(segments[segment_id-1]) == interfaces.end())
						cout << "SuperCell " << cell_id <<", SubCell " << segment_id << "(" <<  segments[segment_id] << "): Interface to segment " << segment_id-1 << "(" << segments[segment_id-1] << ") missing !" << endl;
					else
						cout << "SuperCell " << cell_id <<", SubCell " << segment_id << "(" <<  segments[segment_id] << "): " <<  interfaces.find(segments[segment_id-1])->second  << "  interfaces to  " << segment_id-1 << "(" << segments[segment_id-1] << ") vanish! " << endl; 
					return false;
				}
			}
			if (segment_id+1 < segments.size()) {
				if (updated_interfaces.find(segments[segment_id+1]) == updated_interfaces.end()) {
					if (interfaces.find(segments[segment_id+1]) == interfaces.end())
						cout << "SuperCell " << cell_id <<", SubCell " << segment_id << "(" <<  segments[segment_id] << "): Interface to segment " << segment_id+1 << "(" << segments[segment_id+1] << ") missing !" << endl;
					else
						cout << "SuperCell " << cell_id <<", SubCell " << segment_id << "(" <<  segments[segment_id] << "): " <<  interfaces.find(segments[segment_id+1])->second  << "  interfaces to  " << segment_id+1 << "(" << segments[segment_id+1] << ") vanish! " << endl; 
					return false;
				}
			}
		}

// 	}
	return true;
}


double Rod_Mechanics::delta( CPM::CELL_ID cell_id, const CPM::UPDATE& update, CPM::UPDATE_TODO todo) const
{
	const SuperCell& cell = static_cast<const SuperCell& >(CPM::getCell(cell_id));

// 	static uint lattice_dimensions =  SIM::getLattice()->getDimensions();
	static double boundary_per_node = update.interaction->getStencil().size() / 2; // - sqrt(SIM::getCPMBoundaryNeighborhood().size());
	const vector<VDOUBLE>& pre_update_centers = cell.getSubCenters();
	const vector<VDOUBLE>& post_update_centers = cell.getUpdatedSubCenters();
	
	uint volume(0);
	if ( todo & CPM::ADD )
		volume = CPM::getCell(update.add_state.cell_id).nNodes();
	else // ( todo & CPM::REMOVE)
		volume = CPM::getCell(update.remove_state.cell_id).nNodes();
	
	static vector<double> interface_length;
	static vector<double> size_scale; 
	if (interface_length.size() <= volume ) {
		for (uint i=interface_length.size(); i<= volume; i++) {
			if (lattice_dimensions == 2 ) {
				// Assume a square shape of the cell and calculate extends accordingly. 
				size_scale.push_back(sqrt (double(i) )); 
				interface_length.push_back(sqrt (double(i) * interface_length_factor));
			}
			else if (lattice_dimensions == 3) {
				// Assume a cubic segment shape
				size_scale.push_back(pow ( double( i ), 1.0 / 3.0 ) * 1.2);
				interface_length.push_back(pow ( double( i ), 2.0 / 3.0 ) * interface_length_factor);
			}
		}
	}
	double dE = 0;
	uint first_updated_segment, last_updated_segment;
	vector<double> *energy_buffer;
	
	if (todo == CPM::ADD_AND_REMOVE) {
		assert(update.add_state.super_cell_id == update.remove_state.super_cell_id);
		energy_buffer = &rod_energy(update.add_state.super_cell_id);
		first_updated_segment = CPM::getCellIndex(update.add_state.cell_id).sub_cell_id;
		last_updated_segment = CPM::getCellIndex(update.remove_state.cell_id).sub_cell_id;
		if (first_updated_segment > last_updated_segment ) {
			uint tt = last_updated_segment;
			last_updated_segment = first_updated_segment;
			first_updated_segment = tt;
		}
		// we just discard the interface change in any other case, since they will only rarely happen
		// That's precisely not the case !! 
		if ( last_updated_segment - first_updated_segment == 1 ) {
			const Cell& subcell1 = CPM::getCell(update.add_state.cell_id);
			double i1 = subcell1.getInterfaces().find(update.remove_state.cell_id)->second / boundary_per_node;
			double i2 = subcell1.getUpdatedInterfaces().find(update.remove_state.cell_id)->second / boundary_per_node;
			dE+= interface_strength * ( sqr(i2-interface_length[volume]) - sqr(i1-interface_length[volume]) );
// 			cout << "i1 " << i1 << " i2 " <<i2 << " dE "<< dE << endl;
		}
	}
	else if (todo == CPM::ADD) {
		energy_buffer = &rod_energy(update.add_state.super_cell_id);
		last_updated_segment = first_updated_segment = CPM::getCellIndex(update.add_state.cell_id).sub_cell_id;
		const Cell& subcell1 = CPM::getCell(update.add_state.cell_id); 
		uint segment_id = cell.getSubCellPosition(update.add_state.cell_id);
		const vector<CPM::CELL_ID>& segments = cell.getSubCells();
		if (segment_id>0) {
			double i1 = subcell1.getInterfaces().find(segments[segment_id-1])->second / boundary_per_node;
			double i2 = subcell1.getUpdatedInterfaces().find(segments[segment_id-1])->second / boundary_per_node;
			dE+= interface_strength * ( sqr(i2-interface_length[volume]) - sqr(i1-interface_length[volume]));
		}
		if (segment_id+1 < segments.size()) {
			double i1 = subcell1.getInterfaces().find(segments[segment_id+1])->second / boundary_per_node;
			double i2 = subcell1.getUpdatedInterfaces().find(segments[segment_id+1])->second / boundary_per_node;
			dE+= interface_strength * ( sqr(i2-interface_length[volume]) - sqr(i1-interface_length[volume]));
		}
	}
	else if (todo == CPM::REMOVE) {
		energy_buffer = &rod_energy(update.remove_state.super_cell_id);
		last_updated_segment = first_updated_segment = CPM::getCellIndex(update.remove_state.cell_id).sub_cell_id;
		const Cell& subcell1 = CPM::getCell(update.remove_state.cell_id);
		uint segment_id = cell.getSubCellPosition(update.remove_state.cell_id);
		const vector<CPM::CELL_ID>& segments = cell.getSubCells();
		if (segment_id>0) {
			double i1 = subcell1.getInterfaces().find(segments[segment_id-1])->second / boundary_per_node;
			double i2 = subcell1.getUpdatedInterfaces().find(segments[segment_id-1])->second / boundary_per_node;
			dE+= interface_strength * ( sqr(i2-interface_length[volume]) - sqr(i1-interface_length[volume]));
		}
		if (segment_id+1 < segments.size()) {
			double i1 = subcell1.getInterfaces().find(segments[segment_id+1])->second / boundary_per_node;
			double i2 = subcell1.getUpdatedInterfaces().find(segments[segment_id+1])->second / boundary_per_node;
			dE+= interface_strength * ( sqr(i2-interface_length[volume]) - sqr(i1-interface_length[volume]));
		}
	}
// 	if (energy_buffer->size() != cell.getSubCells().size()) {
// 		
// 	}
	double dEMech =  hamiltonian(post_update_centers,first_updated_segment,last_updated_segment, size_scale[volume]) 
	               - hamiltonian(pre_update_centers, first_updated_segment,last_updated_segment, size_scale[volume]);
// 	if (dEMech == 0 && SIM::getTime() > 1000) {
// 		cout << "  dEMech " << dEMech << endl;
// 		dEMech =  hamiltonian(post_update_centers,first_updated_segment,last_updated_segment, size_scale) 
// 	            - hamiltonian(pre_update_centers, first_updated_segment,last_updated_segment, size_scale);
// 	}
	dE+=dEMech;

	return dE;
}

// double Rod_Mechanics::interaction(double base_interaction, const CPM::STATE& State_a, const CPM::STATE& State_b) const {
// 	if (State_a.cell == State_b.cell && abs(State_a.segment == State_b.segment) == 1 ) return base_interaction*0.2;
// 	return base_interaction;
// 
// }

double Rod_Mechanics::hamiltonian(CPM::CELL_ID cell_id) const
{
	const SuperCell&  cell  = static_cast<const SuperCell&>(CPM::getCell(cell_id));
	double size_scale = sqrt(CPM::getCell(cell.getSubCells()[0]).getNodes().size());
	return hamiltonian(cell.getSubCenters(), 0, cell.getSubCenters().size()-1,size_scale);
}; 

void Rod_Mechanics::update_notify(CPM::CELL_ID cell, const CPM::UPDATE& update, CPM::UPDATE_TODO todo) {
	const SuperCell& sc = static_cast< const SuperCell& >( CPM::getCell(cell) );
	const vector<CPM::CELL_ID>& segments = sc. getSubCells();
	const vector <VDOUBLE >& centers = sc. getSubCenters();
// 	cout << segments.size() << "  "  << centers.size() << endl;;
	assert(segments.size() == centers.size());
// 	double rev_factor = cell->getProperty(reversal_phase_property_id).d >= M_PI ? -1.0 : 1.0;
	if ( ! reversed.valid() || reversed.get(cell) == 0 ) {
		if (segments.size() == 2) {
			orientation.set( segments[0], (centers[0] - centers[1] ) . norm());
			orientation.set( segments[1], (centers[0] - centers[1] ) . norm());
		}
		if (segments.size() > 2) {
			for ( uint i=1; i< segments.size()-1; i++ ) {
				orientation.set( segments[i], (centers[i-1] - centers[i+1] ) . norm());
			}
			orientation.set( segments[0], orientation.get( segments[1] ) );
			orientation.set( segments.back(), orientation.get( segments[segments.size()-2] ));
		}
	}
	else {
		if (segments.size() == 2) {
			orientation.set( segments[0], (centers[1] - centers[0] ) . norm());
			orientation.set( segments[1], (centers[1] - centers[0] ) . norm());
		}
		if (segments.size() > 2) {
			for ( uint i=1; i< segments.size()-1; i++ ) {
				orientation.set( segments[i], (centers[i+1] - centers[i-1] ) . norm());
			}
			orientation.set( segments[0], orientation.get( segments[1] ) );
			orientation.set( segments.back(), orientation.get( segments[segments.size()-2] ));
		}
	}
}

// void Rod_Mechanics::mcs_notify(uint mcs) {
// 	double rev_delta =  2* M_PI / reversal_time * SIM::getMCSDuration();
// 	if (do_reversal) {
// 		for(int c_id=0; c_id<celltype->getNumberOfCells();c_id++ ){
// 			celltype->getCell(c_id).getProperty(reversal_phase_property_id).d += rev_delta * ( 1.0 +  getRandomGauss(0.1));
// 			if (celltype->getCell(c_id).getProperty(reversal_phase_property_id).d >= 2 * M_PI) celltype->getCell(c_id).getProperty(reversal_phase_property_id).d-= 2*M_PI;
// 		}
// 	}
// }

double  Rod_Mechanics::hamiltonian(const vector<VDOUBLE>& centers, uint first_seg, uint last_seg, double size_scale) const
{
	// need a scaling factor for segment dimensions  --> pow(volume, 1.0/dimensions);
	double H_axial = 0;
	
	if (centers.size() > 1) {
		first_seg = first_seg > 0 ? first_seg -1 : 0;
		if ( last_seg + 2 > centers.size() ) last_seg = centers.size()-2;
		for (uint i_seg = first_seg; i_seg <= last_seg; i_seg++) {
			// size scale is the expected radius of a segment
// 			cout << (centers[i_seg]-centers[i_seg+1]).abs() << " | ";
			H_axial += sqr(size_scale -(centers[i_seg]-centers[i_seg+1]).abs());
		}
	}
	double H_bend=0;
	if (centers.size() > 2) {
		first_seg = first_seg > 0 ? first_seg -1 : 0;
		if ( last_seg + 3 > centers.size() ) last_seg = centers.size()-3;

		if ( stiffness_model == tube ) {
			for (uint i_seg = first_seg; i_seg <= last_seg; i_seg++) {
				H_bend += log( bending_parameter * curve3P(centers.begin() + i_seg) / (size_scale * size_scale)+1);
			}
		}
		else if ( stiffness_model == solid ) {
			for (uint i_seg = first_seg; i_seg <= last_seg; i_seg++) {
				H_bend += curve3P(centers.begin() + i_seg) / (size_scale * size_scale);
			}
		}
		else if ( stiffness_model == exponential ) {
			for (uint i_seg = first_seg; i_seg <= last_seg; i_seg++) {
				H_bend += pow(curve3P(centers.begin() + i_seg) / (size_scale * size_scale), bending_parameter );
			}
		}
	}
	return (axial_stiffness * size_scale) * H_axial + bending_stiffness * (size_scale * H_bend);
}

double  Rod_Mechanics::curve3P(vector<VDOUBLE>::const_iterator p) const {
	// solves a normalized problem and rescales afterwards
	VDOUBLE AplusB=((*p-*(p+1)).norm() + (*(p+2)-*(p+1)).norm());
	//curvature => returns 1/R², where R is the curvature radius 
	return (AplusB.x*AplusB.x + AplusB.y*AplusB.y + AplusB.z*AplusB.z) ;
}

REGISTER_PLUGIN(Rod_Segment_Interaction);


void Rod_Segment_Interaction::loadFromXML(const XMLNode Node) {
	Plugin::loadFromXML(Node);
	string interaction_type_name;
	getXMLAttribute(Node,"type",interaction_type_name);
	if (interaction_type_name == "head-adhesion")
		interaction_type = HeadAdhesion;
	else
		interaction_type = ZeroNeighbors; 

}



double Rod_Segment_Interaction::interaction(CPM::STATE s1, CPM::STATE s2, double base_interaction) {
// 	cout << "same cell interaction " << s1.super_cell_id << "  " << s2.super_cell_id <<  endl;
	if (interaction_type == HeadAdhesion) {
 		uint pos1 = CellType::storage.index(s1.cell_id).sub_cell_id;
		uint pos2 = CellType::storage.index(s2.cell_id).sub_cell_id;
		if ( (pos1 == 0 || pos1 == 9) && (pos2==0 || pos2==9) ) {
			return base_interaction;
		}
		return 0;
	}
	else {	
		if ( s1.super_cell_id == s2.super_cell_id) {
			uint pos1 = CellType::storage.index(s1.cell_id).sub_cell_id;
			uint pos2 = CellType::storage.index(s2.cell_id).sub_cell_id;
	// 		uint pos1 = static_cast<const SuperCell&>(CPM::getCell(s2.super_cell_id)).getSubCellPosition(s1.cell_id);
	// 		uint pos2 = static_cast<const SuperCell&>(CPM::getCell(s2.super_cell_id)).getSubCellPosition(s2.cell_id);
	// 		cout << "same cell interaction" << endl;
			if ( pos1+1 == pos2 ||  pos1 == pos2+1 )  return 0;
			
		}
	}
	return base_interaction;
}

