#include "initspherecell.h"

using namespace SIM;

Plugin* initspherecell::createInstance()				//erzeugen einer neuen instanz
{return new initspherecell();};

bool initspherecell::factory_registration = registerPlugin<initspherecell>();						//registrieren des neuen plugins

//============================================================================

initspherecell::initspherecell()							//kreis-initialisierer
{};

void initspherecell::loadFromXML(const XMLNode node)	//einlesen der daten aus der xml
{
    string mode_str;
	getXMLAttribute(node,"mode", mode_str);
	
	if( mode_str == "order" )
		mode = initspherecell::ORDER;
	else if( mode_str == "distance" )
		mode = initspherecell::DISTANCE;
	else 
		mode = initspherecell::DISTANCE;
	
    for (int i=0; i < node.nChildNode("Sphere"); i++) {
        XMLNode cNode = node.getChildNode("Sphere",i);
		Sphere c;
        getXMLAttribute(cNode,"center", c.center);
        getXMLAttribute(cNode,"radius", c.radius);
        spheres.push_back(c);
    }
}



//============================================================================

int initspherecell::setNodes(CellType* ct) //methode zum errechnen der benötigten kreisbahnen
{

    shared_ptr<const Lattice> lattice = SIM::getLattice();

    VINT pos;
    uint i=0;
    for(pos.z = 0; pos.z < lattice->size().z ; pos.z++){
        for(pos.x = 0; pos.x < lattice->size().x ; pos.x++){
            for(pos.y = 0; pos.y < lattice->size().y ; pos.y++){
				
				// first, check whether multiple spheres claim this node
				vector<Candidate> candidates;
				Candidate cand;
                for(int n = 0; n < spheres.size() ; n++){
                    VDOUBLE center = spheres[n].center;
					cand.index = n;
					cand.distance = getLattice()->orth_distance( pos , center ).abs();
                    if( cand.distance < spheres[n].radius ){
						candidates.push_back( cand );
                    }
                }

                // if multiple nodes, let first one (ORDER) or closest one (DISTANCE) have the nodes
                if( candidates.size() ){
					if( candidates.size() == 1 )
						CPM::setNode(pos, CellType::storage.cell(spheres[ candidates[0].index ].id).getID());
					else if ( candidates.size() > 1 ){
						
						if( mode == ORDER ) // first one wins
							CPM::setNode(pos, CellType::storage.cell(spheres[ candidates[0].index ].id).getID());
						else if( mode == DISTANCE ){ // closest one wins
							uint winner;
							double min_dist = 99999.9;
							for(uint c=0; c<candidates.size();c++){
								if( candidates[c].distance < min_dist )
									min_dist = candidates[c].distance;
									winner = c;
							}

							CPM::setNode(pos, CellType::storage.cell(spheres[ candidates[winner].index ].id).getID());
						}
						
						
					}
					i++;
				}
            }
        }
    }
    return i;
}


//============================================================================

bool initspherecell::run(CellType* ct)					//start & auswahl der initialisierungsmethode
{
    for(int n = 0; n < spheres.size() ; n++){
        uint newID = ct->createCell();
        spheres[n].id = newID;
		cout << "Sphere " << n << " = center: " << spheres[n].center << ", radius: " << spheres[n].radius << ", id " << spheres[n].id << endl;

    }

    int i = setNodes(ct);
    cout << "initspherecell: " << i << " nodes added to cell " << endl;
	return true;
}
