#include "interaction_energy.h"
#include "cpm_shape.h"

InteractionEnergy::InteractionEnergy() : interaction_details(IA_PLAIN), n_celltypes(0) {
}


void InteractionEnergy::loadFromXML( const XMLNode xNode, Scope* scope) {
	Plugin::loadFromXML(xNode,scope);
	ia_XMLNode = xNode;
	
	
	//assert( ! ia_neighborhood_node.isEmpty() );
// 	ia_neighborhood = CPM::getBoundaryNeighborhood();
// 	if (ia_XMLNode.nChildNode("Neighborhood"))
// 		ia_neighborhood = SIM::lattice().getNeighborhood(ia_XMLNode.getChildNode("Neighborhood"));

// 	if (ia_neighborhood.empty()) {
// 		cerr << "CPM: Unable to get a neighborhood specification for interaction energy" << endl;
// 		exit(-1);
// 		assert(0);
// 	}
	
// 	cout << "CPM: InteractionEnergy initialized with " << ia_neighborhood.size() << " interaction neighbors" << endl;

	default_interaction = 0;
	getXMLAttribute(xNode,"default", default_interaction);
	
	negate_interactions = false;
	getXMLAttribute(xNode,"negative",negate_interactions);
	
	bool inter_collapse = true;
	getXMLAttribute(xNode,"collapse",inter_collapse);
	if (inter_collapse)
		interaction_details |= IA_COLLAPSE;
	
	// collect all predefined interaction energies
	// we defer reading Contacts to the init phase, where CellTypes are known 
	// - shit, but the it's too late for symbol registration
	// - ohh, so let's just init the as the very first in the sequence ...

	auto celltypes = CPM::getCellTypesMap();
	// map celltype names to internal state id
// 	std::map<std::string,uint> ct_names;
// 	bool has_supercells = false;
// 	for (uint ict=0; ict<celltypes.size(); ict++) {
// 		auto ct = celltypes[ict].lock();
// 		ct_names[ct->getName()]=ict;
// #ifdef HAVE_SUPERCELLS
// 		if (dynamic_pointer_cast<const SuperCT>(ct) )
// 			has_supercells = true;
// #endif
// 	}
	
	n_celltypes=celltypes.size();
	// NOTE we should preferably have a container where the key consists of the two celltype_ids for the interaction energies, e.g. hash_map<struct {uint, uint}, double>
// 	ia_energies.resize(n_celltypes*n_celltypes, default_interaction);
	ia_energies2.resize(n_celltypes*n_celltypes, { make_shared<ThreadedExpressionEvaluator<double>>(to_str(default_interaction),scope), false, false });
	ia_overrider.resize(n_celltypes*n_celltypes);
	plugins.resize(n_celltypes*n_celltypes);
	ia_addon.resize(n_celltypes*n_celltypes);
	
	// collect all predefined interaction energies
	bool has_addons = false;
	bool has_overriders = false;
	uint nContacts = ia_XMLNode.nChildNode("Contact");
	for (uint i=0; i<nContacts; i++) {
		XMLNode xContact = ia_XMLNode.getChildNode("Contact",i);

		// check for sufficient entries
// 		double value; 
		string ct_name1, ct_name2, value_string;
		getXMLAttribute(xContact,"type1",ct_name1);
		auto ct1_i = celltypes.find(ct_name1);
		if ( ct1_i == celltypes.end() ) {
			throw MorpheusException(string("Unable to find cell type ")+ct_name1, xContact);
		}
		getXMLAttribute(xContact,"type2",ct_name2);
		auto ct2_i = celltypes.find(ct_name2);
		if ( ct2_i == celltypes.end() ) {
			throw MorpheusException(string("Unable to find cell type ")+ct_name2, xContact);
		}
// 		if ( ! getXMLAttribute(xContact,"value", value)) {
// 			throw MorpheusException(string("Unable to find interaction value"), xContact);
// 		}
		if ( ! getXMLAttribute(xContact,"value", value_string)) {
			throw MorpheusException(string("Unable to find interaction value"), xContact);
		}

		auto ct1 = ct1_i->second.lock();
		auto ct2 = ct2_i->second.lock();
		uint v_id_1_2 = getInterActionID(ct1->getID(), ct2->getID());
		uint v_id_2_1 = getInterActionID(ct2->getID(), ct1->getID());
		
// 		ia_energies[v_id_1_2] = value;
// 		if (v_id_1_2 != v_id_2_1)
// 			ia_energies[v_id_2_1] = value;
		
		auto evaluator = make_shared<ThreadedExpressionEvaluator<double>>(value_string, scope);
		evaluator->addNameSpaceScope("cell1", ct1->getScope());
		evaluator->addNameSpaceScope("cell2", ct2->getScope());
		ia_energies2[v_id_1_2] = { evaluator, false, true };
		if (v_id_1_2 != v_id_2_1)
			ia_energies2[v_id_2_1] = { evaluator, true, true };
		
		for ( int i_plug =0; i_plug <xContact.nChildNode(); i_plug++) {
			// try to create a suitable plugin
			XMLNode xNode = xContact.getChildNode(i_plug);
			try {
				string xml_tag_name(xNode.getName());
				shared_ptr<Plugin>p = PluginFactory::CreateInstance(xml_tag_name);
				if (! p.get()) 
					throw(string("Unknown plugin " + xml_tag_name));
				p->loadFromXML(xNode, scope);
				
				// TODO: check symbol creation
				if ( dynamic_pointer_cast<CPM_Interaction_Overrider>(p) ) {
					if ( ia_overrider[v_id_1_2])
						throw(string("Multiple interaction overriders for " + ct_name1 + " , " + ct_name2 + " defined!"));
					ia_overrider[ v_id_1_2 ] = dynamic_pointer_cast<CPM_Interaction_Overrider>(p);
					if (v_id_1_2 != v_id_2_1) 
						ia_overrider[ v_id_2_1 ] = dynamic_pointer_cast<CPM_Interaction_Overrider>(p);
					cout << "Registering Interaction overrider " << p->XMLName() << endl;
					has_overriders = true;
				}
				if (dynamic_pointer_cast<CPM_Interaction_Addon>(p) ) {
					ia_addon[ v_id_1_2 ].push_back(dynamic_pointer_cast<CPM_Interaction_Addon>(p) );
					// IF NOT HOMOTYPIC INTERACTION
					if (v_id_1_2 != v_id_2_1) 
						ia_addon[ v_id_2_1 ].push_back( dynamic_pointer_cast<CPM_Interaction_Addon>(p) );
					cout << "Registering Interaction plugin " << p->XMLName() << endl;
					has_addons = true;
				}

				plugins[ v_id_1_2 ].push_back( p );
				// IF NOT HOMOTYPIC INTERACTION
				if (v_id_1_2 != v_id_2_1)  
					plugins[ v_id_2_1 ].push_back(p);
			}
			catch(string er) { 
				throw MorpheusException(er, ia_XMLNode);
				//cerr << er << " - leaving you alone ..." << endl; exit(-1);
			}
			
		}
// 		cout << "Contact (" << celltypes[ct_id1] -> getName() << "," << celltypes[ct_id2] -> getName() << ") => " << value;
// 		cout<< endl;
	}
	
	
	if (has_addons || has_overriders)
		interaction_details |= IA_PLUGINS;

}

XMLNode InteractionEnergy::saveToXML() const {
	return ia_XMLNode;
}

void InteractionEnergy::init(const Scope* scope)
{
	layer = CPM::getLayer();
	
	ia_neighborhood = CPM::getBoundaryNeighborhood().neighbors();
	boundaryLenghScaling = CPMShape::BoundaryLengthScaling( CPM::getBoundaryNeighborhood());
	int origin_offset = layer->get_data_index(VINT(0,0,0));
	for (auto neighbor : ia_neighborhood) {
		ia_neighborhood_offsets.push_back(layer->get_data_index(neighbor) - origin_offset);
	}
	
	auto celltypes = CPM::getCellTypes();
	
	for (auto& interaction : ia_energies2) {
		interaction.value->init();
		registerInputSymbols( interaction.value->getDependSymbols() );
		// optimization expressions not making use of the cell namespaces
		if (interaction.subscope_use) {
			if (interaction.value->getNameSpaceUsedSymbols(0).size()==0 && interaction.value->getNameSpaceUsedSymbols(1).size()==0) {
				interaction.subscope_use = false;
			}
		}
	}
		
	for (uint i=0;i<n_celltypes; i++) {
		if (ia_overrider[i]) {
			ia_overrider[i]->init(celltypes[i].lock()->getScope());
			registerInputSymbols( ia_overrider[i]->getDependSymbols() );
		}
		
		for (uint j=i; j<n_celltypes; j++) {
			for (uint k=0; k < plugins[getInterActionID(i,j)].size(); k++) {
				plugins[getInterActionID(i,j)][k]->init(scope);
				registerInputSymbols( plugins[getInterActionID(i,j)][k]->getDependSymbols() );
			}
		}
	}
	
	cout << "CPM: InteractionEnergy has "
	     << CPM::getBoundaryNeighborhood().size() << " neighbors" << endl;
	cout << "CPM: InteractionEnergy was initialized with "
	     << ((interaction_details & IA_COLLAPSE) ? " IA_COLLAPSE ": "") 
	     << ((interaction_details & IA_PLUGINS) ? " IA_PLUGINS ": "")
	     << ((interaction_details & IA_SUPERCELLS) ? " IA_SUPERCELLS ": "")
	     << endl;
	
}


double InteractionEnergy::getBaseInteraction(const SymbolFocus& cell1, const SymbolFocus& cell2) const {
	int ia_id = getInterActionID(cell1.celltype(), cell2.celltype());
// 	return ia_energies[ia_id];
	const auto& value = *ia_energies2[ia_id].value;
	if (ia_energies2[ia_id].subscope_use) {
		if (ia_energies2[ia_id].celltypes_swapped) {
			value.setNameSpaceFocus(0,cell2);
			value.setNameSpaceFocus(1,cell1);
		}
		else {
			value.setNameSpaceFocus(0,cell1);
			value.setNameSpaceFocus(1,cell2);
		}
	}
	return value.get(cell1);
}


double InteractionEnergy::delta(const CPM::Update& update) const {
	// get Neighborhood stats
#ifdef VTRACE
	VT_TRACER("InteractionEnergy::delta");
#endif
	double dH = 0;
	
	if (interaction_details & IA_COLLAPSE) {
		// Compute Interaction energies on the basis of boundary kernel/stencil stats (relative positional information lost)
		const vector<StatisticalLatticeStencil::STATS>& nei_cells = update.boundaryStencil()->getStatistics();
#ifdef HAVE_SUPERCELLS
		if (interaction_details & IA_SUPERCELLS) {
			if (interaction_details & IA_PLUGINS) {
				CPM::STATE neighbor_state;
				neighbor_state.pos = update.focus().pos();

				for (uint i=0; i<nei_cells.size(); ++i ) {
					neighbor_state.cell_id = nei_cells[i].cell;
					const CPM::INDEX& neighbor_index = CPM::getCellIndex( neighbor_state.cell_id );
					neighbor_state.super_cell_id=neighbor_index.super_cell_id;
					
					if (update.focusStateBefore().cell_id != neighbor_state.cell_id ) {
						uint ia_id = getInterActionID( remove_index.status == CPM::SUB_CELL && remove_index.super_cell_id != neighbor_index.super_cell_id ? remove_index.super_celltype : remove_index.celltype ,
													neighbor_index.status == CPM::SUB_CELL  && remove_index.super_cell_id != neighbor_index.super_cell_id ? neighbor_index.super_celltype : neighbor_index.celltype );
						double interaction_remove;
						if (ia_overrider[ia_id])
							interaction_remove = ia_overrider[ia_id]->interaction(update.focusStateBefore(), neighbor_state,ia_energies[ia_id]);
						else
							interaction_remove = ia_energies[ia_id];
						
						for (uint k=0; k<ia_addon[ia_id].size(); ++k) {
							interaction_remove += ia_addon[ia_id][k]->interaction(update.focusStateBefore(), neighbor_state);
						}
						
						dH -= interaction_remove * nei_cells[i].count;
					}
					if (update.focusStateAfter().cell_id != neighbor_state.cell_id) {
						uint ia_id = getInterActionID( add_index.status == CPM::SUB_CELL && add_index.super_cell_id != neighbor_index.super_cell_id ? add_index.super_celltype : add_index.celltype ,
													neighbor_index.status == CPM::SUB_CELL  && add_index.super_cell_id != neighbor_index.super_cell_id ? neighbor_index.super_celltype : neighbor_index.celltype );
						double interaction_add;
						if (ia_overrider[ia_id])
							interaction_add = ia_overrider[ia_id]->interaction(update.focusStateAfter(), neighbor_state,ia_energies[ia_id]);
						else
							interaction_add = ia_energies[ia_id];
						
						for (uint k=0; k<ia_addon[ia_id].size(); ++k) {
							interaction_add += ia_addon[ia_id][k]->interaction(update.focusStateAfter(), neighbor_state);
						}
						dH += interaction_add * nei_cells[i].count;
					}
				}
			}
			else {
				for (uint i=0; i<nei_cells.size(); ++i ) {
					const CPM::INDEX& neighbor_index = CPM::getCellIndex( nei_cells[i].cell );
					if (update.focusStateBefore().cell_id != nei_cells[i].cell ) {
						uint ia_id = getInterActionID( remove_index.status == CPM::SUB_CELL && remove_index.super_cell_id != neighbor_index.super_cell_id ? remove_index.super_celltype : remove_index.celltype ,
													neighbor_index.status == CPM::SUB_CELL  && remove_index.super_cell_id != neighbor_index.super_cell_id ? neighbor_index.super_celltype : neighbor_index.celltype );
						dH -= ia_energies[ia_id] * nei_cells[i].count;
					}
					if (update.focusStateAfter().cell_id != nei_cells[i].cell) {
						uint ia_id = getInterActionID( add_index.status == CPM::SUB_CELL && add_index.super_cell_id != neighbor_index.super_cell_id ? add_index.super_celltype : add_index.celltype ,
													neighbor_index.status == CPM::SUB_CELL  && add_index.super_cell_id != neighbor_index.super_cell_id ? neighbor_index.super_celltype : neighbor_index.celltype );
						
						dH += ia_energies[ia_id] * nei_cells[i].count;
					}
				}
			}
		}
		else {
#endif // HAVE_SUPERCELLS
			if (interaction_details & IA_PLUGINS) {
				
				for (uint i=0; i<nei_cells.size(); ++i ) {
					SymbolFocus neighbor_focus(nei_cells[i].cell, update.focus().pos());
					
					if (update.focus().cellID() != nei_cells[i].cell ) {
						double interaction_remove = getBaseInteraction(update.focus(), neighbor_focus);
						uint ia_id = getInterActionID(update.focus().celltype(), neighbor_focus.celltype());
						if (ia_overrider[ia_id])
							interaction_remove = ia_overrider[ia_id]->interaction(update.focus(), neighbor_focus, interaction_remove);

						for (uint k=0; k<ia_addon[ia_id].size(); ++k) {
							interaction_remove  += ia_addon[ia_id][k]->interaction(update.focus(), neighbor_focus );
						}
						dH -= interaction_remove * nei_cells[i].count;
					}
					if (update.focusUpdated().cellID() != nei_cells[i].cell) {
						double interaction_add = getBaseInteraction(update.focusUpdated(), neighbor_focus);
						uint ia_id = getInterActionID(update.focusUpdated().celltype(), neighbor_focus.celltype());
						if (ia_overrider[ia_id])
							interaction_add = ia_overrider[ia_id]->interaction(update.focusUpdated(), neighbor_focus, interaction_add);

						for (uint k=0; k<ia_addon[ia_id].size(); ++k) {
							interaction_add += ia_addon[ia_id][k]->interaction(update.focusUpdated(), neighbor_focus );
						}
						dH += interaction_add * nei_cells[i].count;
					}
				}
			}
			else {
				for (uint i=0; i<nei_cells.size(); ++i ) {
					SymbolFocus neighbor_focus(nei_cells[i].cell, update.focus().pos());
					if (update.focusStateBefore().cell_id != nei_cells[i].cell ) {
						dH -= getBaseInteraction(update.focus(), neighbor_focus) * nei_cells[i].count;
					}
					if (update.focusStateAfter().cell_id != nei_cells[i].cell) {
						dH += getBaseInteraction(update.focusUpdated(), neighbor_focus) * nei_cells[i].count;
					}
				}
			}
#ifdef HAVE_SUPERCELLS
		}
#endif
	} else {
		// no collapsed neigbors ...
		int focus_offset = layer->get_data_index(update.focus().pos());
#ifdef __GNUC__
		for (uint k=0; k<ia_neighborhood_row_offsets.size(); ++k ) {
			__builtin_prefetch(&layer->data[ focus_offset + ia_neighborhood_row_offsets[k]],0,1);
		}
#endif
#ifdef HAVE_SUPERCELLS
		if (interaction_details & IA_SUPERCELLS) {
			if (interaction_details & IA_PLUGINS) {
				for (uint i=0; i<ia_neighborhood_offsets.size(); i++) {
					const CPM::STATE& neighbor_state = layer->data[ focus_offset + ia_neighborhood_offsets[i] ];
					const CPM::INDEX& neighbor_index = CellType::storage.index( neighbor_state.cell_id );

					if (update.focusStateBefore().cell_id !=  neighbor_state.cell_id ) {
						uint ia_id = getInterActionID( remove_index.status == CPM::SUB_CELL && remove_index.super_cell_id != neighbor_index.super_cell_id ? remove_index.super_celltype : remove_index.celltype ,
													neighbor_index.status == CPM::SUB_CELL  && remove_index.super_cell_id != neighbor_index.super_cell_id ? neighbor_index.super_celltype : neighbor_index.celltype );
						double interaction_remove = ia_energies[ia_id];
						for (uint k=0; k<ia_addon[ia_id].size(); ++k) {
							interaction_remove += ia_addon[ia_id][k]->interaction(update.focusStateBefore(),  neighbor_state );
						}
						
						dH -= interaction_remove;
					}
					if (update.focusStateAfter().cell_id !=  neighbor_state.cell_id ) {
						uint ia_id = getInterActionID( add_index.status == CPM::SUB_CELL && add_index.super_cell_id != neighbor_index.super_cell_id ? add_index.super_celltype : add_index.celltype ,
													neighbor_index.status == CPM::SUB_CELL  && add_index.super_cell_id != neighbor_index.super_cell_id ? neighbor_index.super_celltype : neighbor_index.celltype );
						double interaction_add = ia_energies[ia_id];
						for (uint k=0; k<ia_addon[ia_id].size(); ++k) {
							interaction_add += ia_addon[ia_id][k]->interaction(update.focusStateAfter(),  neighbor_state );
						}
						dH += interaction_add;
					}
				}
			}
			else {
				for (uint i=0; i<ia_neighborhood_offsets.size(); i++) {
					const CPM::STATE& neighbor_state =layer->data[ focus_offset + ia_neighborhood_offsets[i] ];
					const CPM::INDEX& neighbor_index = CellType::storage.index( neighbor_state.cell_id );
					if (update.focusStateBefore().cell_id !=  neighbor_state.cell_id ) {
						uint ia_id = getInterActionID( remove_index.status == CPM::SUB_CELL && remove_index.super_cell_id != neighbor_index.super_cell_id ? remove_index.super_celltype : remove_index.celltype ,
													neighbor_index.status == CPM::SUB_CELL  && remove_index.super_cell_id != neighbor_index.super_cell_id ? neighbor_index.super_celltype : neighbor_index.celltype );
						dH -= ia_energies[ia_id];
					}
					if (update.focusStateAfter().cell_id != neighbor_state.cell_id ) {
						uint ia_id = getInterActionID( add_index.status == CPM::SUB_CELL && add_index.super_cell_id != neighbor_index.super_cell_id ? add_index.super_celltype : add_index.celltype ,
													neighbor_index.status == CPM::SUB_CELL  && add_index.super_cell_id != neighbor_index.super_cell_id ? neighbor_index.super_celltype : neighbor_index.celltype );
						
						dH += ia_energies[ia_id];
					}
				}
			}
		}
		else {
#endif // HAVE_SUPERCELLS
			// no supercells
			// no collapsed neigbors ...
			if (interaction_details & IA_PLUGINS) {
				CPM::STATE neighbor_state;
				neighbor_state.pos = update.focus().pos();
				
				CPM::setInteractionSurface(true);
				for (uint i=0; i<ia_neighborhood_offsets.size(); i++) {
					SymbolFocus neighbor_focus(layer->data[ focus_offset + ia_neighborhood_offsets[i]].cell_id, update.focus().pos() + ia_neighborhood[i]);
					
					if (update.focus().cellID() != neighbor_focus.cellID()) {
						double interaction_remove = getBaseInteraction(update.focus(), neighbor_focus);
						uint ia_id = getInterActionID(update.focus().celltype(), neighbor_focus.celltype());
						for (uint k=0; k<ia_addon[ia_id].size(); ++k) {
							interaction_remove  += ia_addon[ia_id][k]->interaction(update.focus(), neighbor_focus );
						}
						dH -= interaction_remove;
					}
					if (update.focusUpdated().cellID() != neighbor_focus.cellID() ) {
						double interaction_add = getBaseInteraction(update.focusUpdated(), neighbor_focus);
						uint ia_id = getInterActionID(update.focusUpdated().celltype(), neighbor_focus.celltype());
						for (uint k=0; k<ia_addon[ia_id].size(); ++k) {
							interaction_add += ia_addon[ia_id][k]->interaction(update.focusUpdated(), neighbor_focus );
						}
						dH += interaction_add;
					}
				}
				CPM::setInteractionSurface(false);
			}
			else {
				for (uint i=0; i<ia_neighborhood_offsets.size(); i++) {
					SymbolFocus neighbor_focus(layer->data[ focus_offset + ia_neighborhood_offsets[i]].cell_id, update.focus().pos() + ia_neighborhood[i]);
					if ( update.focus().cellID() != neighbor_focus.cellID() ) {
						dH -= getBaseInteraction(update.focus(), neighbor_focus);
					}
					if ( update.focusUpdated().cellID() != neighbor_focus.cellID() ) {
						dH += getBaseInteraction(update.focusUpdated(), neighbor_focus);
					}
				}
			}
#ifdef HAVE_SUPERCELLS
		}
#endif
	}

	if (negate_interactions) 
		return -dH / boundaryLenghScaling;
	else
		return dH / boundaryLenghScaling;
}


// double InteractionEnergy::delta(const & update) const {
// 	double dHNei=0;
// 	
// 	 add_index = CPM::getCellIndex(update.add_state.cell_id);
// 	 remove_index = CPM::getCellIndex(update.remove_state.cell_id);
// 	
// 	for (vector<VINT>::const_iterator offset  = ia_neighborhood.begin(); offset != ia_neighborhood.end(); ++offset ) {
// 		const CPM::STATE& neighbor = cpm_layer->get( update.focus + (*offset) );
// 		 neighbor_index = CPM::getCellIndex( neighbor.cell_id );
// 
// 		// Energy release due to unbinding of update.remove_state to neighbors
// 		if (update.remove_state != neighbor) {
// 			uint ia_id = getInterActionID(remove_index.celltype, neighbor_index.celltype);
// 			double interaction_remove = ia_energies[ia_id];
// 			for (uint k=0; k<ia_addon[ia_id].size(); ++k) {
// 				interaction_remove += ia_addon[ia_id][k]->interaction(update.remove_state, neighbor);
// 			}
// 			if ( ia_overrider[ia_id]  )
// 				interaction_remove = ia_overrider[ia_id]->interaction(update.remove_state, neighbor, interaction_remove);
// 			dHNei -= interaction_remove;
// 		}
// 
// // Energy gain due to binding update.add_state to neighbors
// 		if (update.add_state != neighbor) {
// 			uint ia_id = getInterActionID(add_index.celltype, neighbor_index.celltype);
// 			double interaction_add = ia_energies[ia_id];
// 			for (uint k=0; k<ia_addon[ia_id].size(); ++k) {
// 				interaction_add += ia_addon[ia_id][k]->interaction(update.add_state, neighbor);
// 			}
// 			if ( ia_overrider[ia_id]  )
// 				interaction_add = ia_overrider[ia_id]->interaction(update.add_state, neighbor, interaction_add);
// 			dHNei += interaction_add;
// 		}
// 		
// 	}
// 
// 	return (dHNei)/ ia_neighborhood.size();
// }


double InteractionEnergy::hamiltonian(const Cell* gc) const {
	double dH=0;
// 	const VINT *Neighbor;
// 	int i=0;
// 	VINT a;

// TODO Implementation missing
	cerr << "missing implementation of InteractionEnergy::hamiltonian" << endl; exit(-1);
	return dH=0;
}
