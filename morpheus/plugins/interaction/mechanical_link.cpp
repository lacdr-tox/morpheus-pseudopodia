#include "mechanical_link.h"

REGISTER_PLUGIN ( MechanicalLink );

// please forgive me. everything in here is quick and dirty

MechanicalLink::MechanicalLink() : InstantaneousProcessPlugin(XMLSpec::XML_NONE), CPM_Energy() {

	strength.setXMLPath("strength");
	registerPluginParameter(strength);

	link_probability.setXMLPath("link-probability");
	registerPluginParameter(link_probability);
	
	unlink_probability.setXMLPath("unlink-probability");
	registerPluginParameter(unlink_probability);
	
	unlink_probability.setLocalsTable({
		{"stretch.abs", EvaluatorVariable::DOUBLE}, 
		{"stretch.rel", EvaluatorVariable::DOUBLE}
	});
}

void MechanicalLink::loadFromXML(const XMLNode node, Scope* scope)
{
	celltype = scope->getCellType();
	bonds = celltype->addProperty<LinkType>("_mechanical_links",{});
	scope->registerSymbol(bonds);
	
	// Require symbols from global scope or named scope definitions
	link_probability.setGlobalScope();
	link_probability.addNameSpaceScope("cell1", celltype->getScope());
	link_probability.addNameSpaceScope("cell2", celltype->getScope());
	// Require symbols from global scope or named scope definitions
	unlink_probability.setGlobalScope();
	unlink_probability.addNameSpaceScope("cell1", celltype->getScope());
	unlink_probability.addNameSpaceScope("cell2", celltype->getScope());
	// Require symbols from global scope or named scope definitions
	strength.setGlobalScope();
	strength.addNameSpaceScope("cell1", celltype->getScope());
	strength.addNameSpaceScope("cell2", celltype->getScope());
	
	InstantaneousProcessPlugin::loadFromXML(node, scope);
	CPM_Energy::loadFromXML(node, scope);
}


void MechanicalLink::init(const Scope* scope) {
	CPM_Energy::init(scope);
	InstantaneousProcessPlugin::init(scope);

	// force MCS time step duration
	setTimeStep( CPM::getMCSDuration() );
	this->is_adjustable = false;
	
	ns1_id = strength.getNameSpaceId("cell1");
	ns2_id = strength.getNameSpaceId("cell2");
	use_ns_strength = ! ( strength.getNameSpaceUsedSymbols(ns1_id).empty() && strength.getNameSpaceUsedSymbols(ns2_id).empty() );
	use_ns_unlink =  ! ( unlink_probability.getNameSpaceUsedSymbols(ns1_id).empty() && unlink_probability.getNameSpaceUsedSymbols(ns2_id).empty() );
	use_ns_link =  ! ( link_probability.getNameSpaceUsedSymbols(ns1_id).empty() && link_probability.getNameSpaceUsedSymbols(ns2_id).empty() );
}

template <class T>
bool contains(const vector<T>& vec, const T& el) { return find(vec.begin(), vec.end(), el) != vec.end(); }

bool MechanicalLink::insertBond(const SymbolFocus& cell_a, const SymbolFocus& cell_b) const
{
	auto& cell_a_reg = bonds->getRef(cell_a);
	auto& cell_b_reg = bonds->getRef(cell_b);
	// assert uniqueness
	if (contains(cell_a_reg,cell_b.cellID())) return false;
	if (contains(cell_b_reg,cell_a.cellID())) return false;
	
	cell_a_reg.push_back(cell_b.cellID());
	cell_b_reg.push_back(cell_a.cellID());
	return true;
}

bool MechanicalLink::removeBond(const SymbolFocus& cell_a, const SymbolFocus& cell_b) const
{
	auto& cell_a_reg = bonds->getRef(cell_a);
	auto& cell_b_reg = bonds->getRef(cell_b);
	// assert uniqueness
	auto cell_a_ref = find(cell_a_reg.begin(), cell_a_reg.end(), cell_b.cellID());
	if (cell_a_ref == cell_a_reg.end()) return false;
	auto cell_b_ref = find(cell_b_reg.begin(), cell_b_reg.end(), cell_a.cellID());
	if (cell_b_ref == cell_b_reg.end()) return false;
	
	cell_a_reg.erase(cell_a_ref);
	cell_b_reg.erase(cell_b_ref);
	return true;
}

void MechanicalLink::executeTimeStep(){
	// cell population of celltype 
	vector<CPM::CELL_ID> cell_ids = celltype->getCellIDs();
	// create and delete bonds to neighbors
	for ( uint i=0; i<cell_ids.size(); i++ ) {

		SymbolFocus cellFocus(cell_ids[i]);
		const auto& cell = cellFocus.cell();
		const vector<CPM::CELL_ID>& cell_bonds = bonds->get(cellFocus);
		// Remove Links from the bonds
		for (auto it_bond = cell_bonds.begin(); it_bond < cell_bonds.end(); ) {
			// assert single processing
			SymbolFocus cellFocusN(*it_bond); it_bond++;
			if (cellFocus.cellID() > *it_bond) continue;
			
			double center_equi_dist = sqrt(cell.getSize()/M_PI) + sqrt(cellFocusN.cell().getSize()/M_PI);
			double center_dist = (cell.getCenter() - cellFocusN.cell().getCenter()).abs();
			double locals[2];
			locals[0] = center_dist - center_equi_dist;   // absolute stretch
			locals[1] =  center_dist / center_equi_dist;  // relative stretch
			unlink_probability.setLocals(locals);
			if (use_ns_unlink) {
				unlink_probability.setNameSpaceFocus(ns1_id,cellFocus);
				unlink_probability.setNameSpaceFocus(ns2_id,cellFocusN);
			}
			
			// probabilistic breakup
			if (unlink_probability(cellFocus) >= getRandom01()) {
				removeBond(cellFocus, cellFocusN);
			}
		}
		
		// Create Links for unlinked direct neighbors
		// iterate through list of neighbors with cell granularity
		const map< CPM::CELL_ID, double >& interfaces =  cell.getInterfaceLengths();
		for (pair< CPM::CELL_ID, double > it_neighbors: interfaces) {
			SymbolFocus cellFocusN(it_neighbors.first);
			// has interface, assert single processing and same celltype
			if (it_neighbors.second > 0 && cellFocus.cellID() < cellFocusN.cellID() && cellFocusN.celltype() == celltype->getID()) {
				if (!contains(cell_bonds, cellFocusN.cellID())) {
					// bond does not exist
					if (use_ns_link) {
						unlink_probability.setNameSpaceFocus(ns1_id,cellFocus);
						unlink_probability.setNameSpaceFocus(ns2_id,cellFocusN);
					}
					if (link_probability(cellFocus) > getRandom01()) {
						insertBond(cellFocus, cellFocusN);
					}
				}
			}
		}
	}
}

double MechanicalLink::delta ( const SymbolFocus& cell_focus, const CPM::Update& /*update*/) const
{

	const Cell& cell = cell_focus.cell();
	VDOUBLE update_displacement = cell.getUpdatedCenter() - cell.getCenter();

	const vector<CPM::CELL_ID>& cell_bonds = bonds->get(cell_focus);
	double dE =0;

	strength.setNameSpaceFocus(ns1_id, cell_focus);
	// iterate through list of neighbors with cell granularity
	for (auto bond : cell_bonds) {
		// check for existing links and add to contribution
		SymbolFocus cellFocusN(bond);
		double center_equi_dist = sqrt(cell.getSize()/M_PI) + sqrt(cellFocusN.cell().getSize()/M_PI);
		VDOUBLE center_dist = (cell.getCenter() - cellFocusN.cell().getCenter());
		if (use_ns_strength)
			strength.setNameSpaceFocus(ns2_id, cellFocusN);
		dE += strength(SymbolFocus::global) * dot(update_displacement, center_dist * ( 1 - center_equi_dist/center_dist.abs()));
	}
	
	return dE;
}

double MechanicalLink::hamiltonian(CPM::CELL_ID /*cell_id*/) const {
	return 0; 
};




