#include "cell_death.h"
#include <map>

REGISTER_PLUGIN(CellDeath);

CellDeath::CellDeath() : InstantaneousProcessPlugin( TimeStepListener::XMLSpec::XML_NONE ) {
	condition.setXMLPath("Condition/text");
	registerPluginParameter(condition);
	target_volume.setXMLPath("Shrinkage/target-volume");
	registerPluginParameter(target_volume);
    remove_volume.setXMLPath("Shrinkage/remove-volume");
    registerPluginParameter(remove_volume);
    replace_mode.setXMLPath("Shrinkage/replace-with");
    map<string, ReplaceMode> modeMap;
    modeMap["random neighbor"]  = CellDeath::ReplaceMode::RANDOM_NB;
    modeMap["longest interface"]  = CellDeath::ReplaceMode::LONGEST_IF;
    modeMap["medium"] = CellDeath::ReplaceMode::MEDIUM;

    //    replace_mode.setDefault("medium");
    replace_mode.setConversionMap(modeMap);
    registerPluginParameter(replace_mode);
}

void CellDeath::init(const Scope* scope)
{
	InstantaneousProcessPlugin::init( scope );
	
	setTimeStep( CPM::getMCSDuration() );
	is_adjustable = false;
	celltype = scope->getCellType();
	if ( target_volume.isDefined() ) {
		mode = SHRINKAGE;
		// determine whether the target_volume symbol has a cellular (spatial) granularity
		if ( target_volume.accessor()->granularity() != Granularity::Cell ){
			throw MorpheusException("CellDeath expects the target-volume symbol to be a cell property!", stored_node);
		}

	}
	else {
		mode = LYSIS;
	}

	// because this plugin changes the cell configuration, 
	//  the cell position need to be registered as output symbol (during init())
	registerCellPositionOutput();
}


CPM::CELL_ID CellDeath::getRandomFusionPartner(std::map<CPM::CELL_ID,double> p_map){
  CPM::CELL_ID fusion_partner_id;
  double p = getRandom01();
  double p_cum = 0;
  for (auto item : p_map){
    if ((p >= p_cum) && (p <= (p_cum + item.second))) {
      fusion_partner_id = item.first;
      break;
    }
    p_cum += item.second;
  }
  return fusion_partner_id;
}


void CellDeath::executeTimeStep()
{
	vector<CPM::CELL_ID> cells = celltype->getCellIDs();
	for (int i=0; i<cells.size(); i++ ) {
		bool remove_cell = false;
		CPM::CELL_ID cell_id = cells[i];
		
		if (dying.find(cell_id) != dying.end()) {

			if (CPM::getCell(cell_id).nNodes() <= remove_volume( SymbolFocus(cell_id) ))
				remove_cell = true;
		}
		else {
			bool about_to_die = condition(SymbolFocus(cell_id, VINT(CPM::getCell( cell_id ).getCenterL())) ) >= 1.0;
			if (about_to_die) {
				//cout << "Triggered cell death for cell " << cell_id << " at " << currentTime() << " due to condition" << endl;
				if (mode == LYSIS) {
					remove_cell = true;
				}
				else if (mode == SHRINKAGE) {
					target_volume.set(SymbolFocus(cell_id),0);
					dying.insert(cell_id);
				}
			}
		}
		
		if (remove_cell) {
			cout << "Removing cell " << cell_id << " at " << currentTime() << endl;
            SymbolFocus cell_focus(cell_id);
            const auto& interfaces = CPM::getCell(cell_id).getInterfaceLengths();
            bool to_medium = false;
            CPM::CELL_ID fusion_partner_id;
            if (replace_mode() == CellDeath::ReplaceMode::MEDIUM) {
              to_medium = true;
            }
            else if (interfaces.size() == 0) {
              to_medium = true;
            }
            else {
              double fusion_interface_length = 0;
              // change to purely random (?)
              if (replace_mode() == CellDeath::ReplaceMode::RANDOM_NB) {
                std::map<CPM::CELL_ID, double> p_map;
                for (auto nb = interfaces.begin(); nb != interfaces.end(); nb++, i++) {
                  p_map.insert(std::pair<CPM::CELL_ID, double>(nb->first,
                                                               nb->second
                                                                   / CPM::getCell(cell_id).getInterfaceLength()));
                }
                fusion_partner_id = getRandomFusionPartner(p_map);
              }
              else if (replace_mode() == CellDeath::ReplaceMode::LONGEST_IF) {
                // find longest interface
                double longest_interface = 0;
                for (auto nb = interfaces.begin(); nb != interfaces.end(); nb++, i++) {
                  if (nb->second >= longest_interface) {longest_interface = nb->second;}
                }
                // find all cells that have this interface length
                std::vector<CPM::CELL_ID> longest;
                for (auto nb = interfaces.begin(); nb != interfaces.end(); nb++, i++) {
                  if (nb->second == longest_interface){longest.push_back(nb->first);}
                }
                // if there is only one longest
                if (longest.size() == 1){
                  fusion_partner_id = longest[0];
                }
                // if there are multiple longest interfaces, choose one at random
                else if(longest.size() > 1){
                  std::map<CPM::CELL_ID, double> p_map;
                  for (auto cell_id : longest){
                    p_map.insert(std::pair<CPM::CELL_ID, double>(cell_id,1./((double)longest.size())));
                  }
                  fusion_partner_id = getRandomFusionPartner(p_map);
                }
              }
              if (fusion_partner_id == CPM::getEmptyCelltypeID()){
                to_medium = true;
              }
            }
            // randomly select neighbor: nb->second/CPM::getCell(cell_id).getInterfaceLength()
            // or select longest interface
            if (to_medium){
              cout << "Dead pixels to medium\n";
              CPM::setCellType(cell_id, CPM::getEmptyCelltypeID());
              if (dying.find(cell_id) != dying.end())
                dying.erase(cell_id);
            }
            else{
              cout << "Dead pixels to " << CPM::getCell(fusion_partner_id).getCellType()->getName() << endl;
              const Cell::Nodes& dying_nodes = CPM::getCell(cell_id).getNodes();
              while (!dying_nodes.empty())
              {
                CPM::setNode( *(dying_nodes.begin()), fusion_partner_id );
              }
              CPM::setCellType( cell_id, CPM::getEmptyCelltypeID() );
            }
            
		}
	}
}

