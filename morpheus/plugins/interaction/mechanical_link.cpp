#include "mechanical_link.h"

REGISTER_PLUGIN ( MechanicalLink );

// please forgive me. everything in here is quick and dirty

MechanicalLink::MechanicalLink() {
	// required
    target_volume.setXMLPath("target-volume");
	registerPluginParameter(target_volume);

    strength.setXMLPath("strength");
    registerPluginParameter(strength);

    link_probability.setXMLPath("link-probability");
    registerPluginParameter(link_probability);

    max_bond_stretch.setXMLPath("max-bond-stretch");
    registerPluginParameter(max_bond_stretch);

}


void MechanicalLink::init(const Scope* scope) {
	CPM_Energy::init(scope);
    InstantaneousProcessPlugin::init(scope);

    setTimeStep( CPM::getMCSDuration() );

	celltype = scope->getCellType();

    cell_number = 0;

    d_0 = 2 * sqrt(target_volume.get( SymbolFocus() ) /M_PI);
}

void MechanicalLink::executeTimeStep(){
    // find biggest cell index to create matrix of that size
    vector<CPM::CELL_ID> cell_ids = celltype->getCellIDs();
    vector<CPM::CELL_ID>::iterator it_cell_ids;
    it_cell_ids = max_element(cell_ids.begin(), cell_ids.end());
    unsigned int updated_cell_number = (unsigned int) cell_ids[distance(cell_ids.begin(), it_cell_ids)];
    unsigned int nb_cell_id;

    bool **new_bonds;
    // if cell number changed, create bigger matrix
    if (updated_cell_number != cell_number){
        // empty initialization
        new_bonds = new bool*[updated_cell_number + 1];
        for (int i = 0; i < updated_cell_number + 1; i++) {
            new_bonds[i] = new bool[updated_cell_number + 1];
            for (int j = 0; j < updated_cell_number + 1; j++) new_bonds[i][j] = false;
        }

        // if old bond matrix already existed, copy old values in
        if (cell_number != 0){
            for (int i = 0; i < cell_number + 1; i++) {
                for (int j = 0; j < cell_number + 1; j++) new_bonds[i][j] = bonds[i][j];
            }
        }
        cell_number = updated_cell_number;
    }
    else
        new_bonds = bonds;

    // store current bond state
    //bonds = new_bonds; // reference - this probably does not do what I want it to do

    // create and delete bonds to neighbors
    for ( uint i=0; i<cell_ids.size(); i++ ) {

        unsigned int cell_id = (unsigned int) cell_ids[i];
        neighbor_set.clear();

        // iterate through list of neighbors with cell granularity
        const map< CPM::CELL_ID, double >& interfaces =  cell.getInterfaceLengths();
        for (pair< CPM::CELL_ID, double > it_neighbors: interfaces) {
            if (it_neighbors.second > 0) {
                const Cell &nb_cell = CPM::getCell(it_neighbors.first);
                nb_cell_id = (unsigned int) it_neighbors.first;
                double norm_dist =
                        (nb_cell.getCenter() - (CPM::getCell((CPM::CELL_ID) cell_id)).getCenter()).abs() - d_0;

                // if bond does not exist, potentially create it
                if ((!new_bonds[cell_id][nb_cell_id]) && (getRandom01() < link_probability.get(SymbolFocus()))) {
                    new_bonds[cell_id][nb_cell_id] = true;
                    new_bonds[nb_cell_id][cell_id] = true;
                }

                // if bond exists, potentially delete it
                else if ((new_bonds[cell_id][nb_cell_id]) &&
                         (getRandom01() < (norm_dist / max_bond_stretch.get(SymbolFocus())))) {
                    new_bonds[cell_id][nb_cell_id] = false;
                    new_bonds[nb_cell_id][cell_id] = false;
                }
                // store neighbor cell IDs
                neighbor_set.insert(nb_cell_id);
            }
        }
        // for row in matrix: if bond exists check occurrence in neighbor set: if no: delete bond
        for (int i = 0; i < cell_number + 1; i++){
            if(new_bonds[cell_id][i]){
                const bool is_in = neighbor_set.find(i) != neighbor_set.end();
                if(!is_in){
                    new_bonds[cell_id][i] = false;
                    new_bonds[cell_id][i] = false;
                }
            }
        }
    }

    bonds = new_bonds;
}

double MechanicalLink::delta ( const SymbolFocus& cell_focus, const CPM::Update& update) const
{

	const Cell& cell = cell_focus.cell();
    VDOUBLE r_i = cell.getUpdatedCenter();
	VDOUBLE update_direction = r_i - cell.getCenter();

    VDOUBLE nb_sum = VDOUBLE();
    VDOUBLE r_ij;

    // iterate through list of neighbors with cell granularity
    const map< CPM::CELL_ID, double >& interfaces =  cell.getInterfaceLengths();
    for (pair< CPM::CELL_ID, double > it_neighbors: interfaces) {
        if (it_neighbors.second > 0) {
            // check for existing links and add to contribution
            const Cell& nb_cell = CPM::getCell(it_neighbors.first);
            if (bonds[(unsigned int) cell.getID()][(unsigned int) nb_cell.getID()] > 0) {
                r_ij = nb_cell.getCenter() - r_i;
                nb_sum += r_ij * (r_ij.abs() - d_0) / r_ij.abs();
            }
        }
    }
    // retracting cell is handled in its own plugin!
	return strength(cell_focus) * dot(update_direction, nb_sum);
}

double MechanicalLink::hamiltonian(CPM::CELL_ID cell_id) const {
	return 0; 
};



