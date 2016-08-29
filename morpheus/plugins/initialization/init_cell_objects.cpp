#include "init_cell_objects.h"

REGISTER_PLUGIN(InitCellObjects);

InitCellObjects::InitCellObjects(){
	mode.setXMLPath("mode");
	map<string, Mode> conv_map;
	conv_map["order"] = ORDER;
	conv_map["distance"] = DISTANCE;
	mode.setConversionMap(conv_map);
	mode.setDefault("distance");
	registerPluginParameter(mode);
};

void InitCellObjects::loadFromXML(const XMLNode node)	//einlesen der daten aus der xml
{	
	Population_Initializer::loadFromXML( node );
	
	VDOUBLE arrange_displacement(0.,0.,0.);
	VINT    arrange_repetitions(1,1,1);
	random_displacement = 0;
	
	for (int i=0; i < node.nChildNode("Arrangement"); i++) {
		XMLNode aNode = node.getChildNode("Arrangement",i);

		getXMLAttribute(aNode,"repetitions", arrange_repetitions);
		arrange_repetitions.x = (arrange_repetitions.x==0 ? 1 : arrange_repetitions.x);
		arrange_repetitions.y = (arrange_repetitions.y==0 ? 1 : arrange_repetitions.y);
		arrange_repetitions.z = (arrange_repetitions.z==0 ? 1 : arrange_repetitions.z);
		
		getXMLAttribute(aNode,"displacements", arrange_displacement);
		
		getXMLAttribute(aNode,"random_displacement", random_displacement);

 		cout << "InitCellObjects: Arrange: displacement: " << arrange_displacement << ", repetitions: " << arrange_repetitions << endl;

		CellObject c;
		for (int j=0; j < aNode.nChildNode("Object"); j++) {
			XMLNode oNode = aNode.getChildNode("Object",j);
			c = getObjectProperties( oNode );
			uint num_before = cellobjects.size();
			arrangeObjectCombinatorial(c, cellobjects, arrange_displacement, arrange_repetitions);
			uint num_after = cellobjects.size();
 			cout << "InitCellObjects: Arranged " << (num_after-num_before) << " objects." << endl;
		}
	}
}

bool InitCellObjects::run(CellType* ct)
{
	lattice = SIM::getLattice();
	for(int n = 0; n < cellobjects.size() ; n++){
		int newID = ct->createCell();
		cellobjects[n].id = newID;
// 		cout << "CellObject " << n << " = center: " << cellobjects[n].center << ", id " << cellobjects[n].id << endl;
	}

	int i = setNodes(ct);
	int unset =0;
	for(int n = 0; n < cellobjects.size() ; n++){
		uint cellsize = ct->getCell(cellobjects[n].id).getNodes().size();
// 		cout << "CellObject " << n << " (" << ct->getName() << "), size = " << cellsize << endl;

		if( cellsize == 0 ){
			cout << "WARNING in InitCellObject: Cell " << n << " has zero size (perhaps due to overlap with other object?). Removing cell " << n << "." << endl;
			ct->removeCell( cellobjects[n].id );
			unset++;
		}
	}
	cout << "InitCellObject: Added " <<  cellobjects.size() - unset << " cell(s), occupying " << i << " nodes" << endl;
	return true;
}


InitCellObjects::CellObject InitCellObjects::getObjectProperties(const XMLNode oNode){
	CellObject c;
	if( oNode.nChildNode("Point") ){
		c.type = POINT;
		getXMLAttribute(oNode,"Point/position", c.origin);
		c.center = c.origin;
	}
	else if( oNode.nChildNode("Sphere") ){
		c.type = SPHERE;
		getXMLAttribute(oNode,"Sphere/center", c.center);
		getXMLAttribute(oNode,"Sphere/radius", c.radius);
	}
    else if( oNode.nChildNode("Ellipsoid") ){
        c.type = ELLIPSOID;
        getXMLAttribute(oNode,"Ellipsoid/center", c.center);
        getXMLAttribute(oNode,"Ellipsoid/axes", c.axes);
		
    }
    else if( oNode.nChildNode("Box") ){
		c.type = BOX;
		getXMLAttribute(oNode,"Box/origin", c.origin);
		getXMLAttribute(oNode,"Box/size", c.boxsize);
		c.center = (c.origin + c.boxsize) / 2.0;
	}
	else if( oNode.nChildNode("Cylinder") ){
		c.type = CYLINDER;
		getXMLAttribute(oNode,"Cylinder/center", c.center);
		getXMLAttribute(oNode,"Cylinder/radius", c.radius);
		string orientation;
		getXMLAttribute(oNode,"Cylinder/orientation", orientation);
		if( orientation == "x") c.orientation = X;
		else if( orientation == "y") c.orientation = Y;
		else c.orientation = Z;
		
		if ( !getXMLAttribute(oNode,"Cylinder/center2", c.center2) )
			c.center2 = c.center;
		
		c.oblique = false;
		if( c.center2 != c.center )
			c.oblique = true;
	}
	VDOUBLE center = c.center;
	c.center = center;
	
	return c;
}


void InitCellObjects::arrangeObjectCombinatorial( CellObject c, vector<CellObject>& objectlist, VDOUBLE displacement, VINT repetitions){

	VINT latsize = SIM::lattice().size();

	VINT r(0,0,0);
	for(r.x=0; r.x<repetitions.x; r.x++){
		for(r.y=0; r.y<repetitions.y; r.y++){
			for(r.z=0; r.z<repetitions.z; r.z++){
				
				CellObject n = c;
				n.center += r*displacement;
// 				n.center = VDOUBLE(n.center.x+double(r.x)*displacement.x,
// 								n.center.y+double(r.y)*displacement.y,
// 								n.center.z+double(r.z)*displacement.z;

				switch(n.type){
					case POINT:
					case BOX:{
						n.origin += r*displacement;
// 						n.origin = n.origin.x+double(r.x)*displacement.x,
// 										n.origin.y+double(r.y)*displacement.y,
// 										n.origin.z+double(r.z)*displacement.z;
						break;
					}
					case CYLINDER:{
						n.center2 += r*displacement;
// 						n.center2 = n.center2.x+double(r.x)*displacement.x,
// 											n.center2.y+double(r.y)*displacement.y,
// 											n.center2.z+double(r.z)*displacement.z;
						break;
					}
				}
				if(random_displacement > 0){
					n.center.x += -random_displacement*2 + getRandom01()*random_displacement;
					if(SIM::lattice().getDimensions() > 1)
						n.center.y += -random_displacement*2 + getRandom01()*random_displacement;
					if(SIM::lattice().getDimensions() > 2)
						n.center.z += -random_displacement*2 + getRandom01()*random_displacement;
				}
 		
				/* TODO: SUPPORT FOR HEXAGONAL LATTICES! */
				
				/* Assert all points fit into the lattice */
				bool resolve_result = true;
				VDOUBLE t;
				t = n.center; resolve_result &= SIM::lattice().orth_resolve(t);
				t = n.center2; resolve_result &= SIM::lattice().orth_resolve(t);
				// TODO : n.origin appears to be interpreted as lattice coordinate. This is somewhat contrasting the other parameters, which are given in orth. coordinates
				t = n.origin; resolve_result &= SIM::lattice().orth_resolve(t);
				
				if( ! resolve_result ) {
					cerr << "InitCellObjects: Error: Cell center " << n.center << " is outside of lattice. " << endl;
					exit(-1);
				}
				
// 				// Switch to lattice coordinates
//  			n.center    = SIM::lattice().from_orth(n.center);
// 				n.center2 	= SIM::lattice().from_orth(n.center2);
// 				n.origin 	= SIM::lattice().from_orth(n.origin);
				
//  				cout  << " n.center: " << n.center << endl;
				
				
				
				objectlist.push_back( n );
// 				cout << r << ", objects: " << objectlist.size() << ", center: " << n.center << endl;
//  				if( n.center.x > latsize.x || n.center.y > latsize.y || n.center.z > latsize.z ||
//  					n.center.x < 0 || n.center.y < 0 || n.center.z < 0){
//   						cerr << "InitCellObjects: Error: Cell center " << n.center << " is outside of lattice. " << endl;
//   						exit(-1);
//   				}
            }
        }
    }
}

//============================================================================

int InitCellObjects::setNodes(CellType* ct)
{
	shared_ptr<const Lattice> lattice = SIM::getLattice();

	VINT pos;
	uint i=0;
	VINT lsize = lattice->size();
	
	for(pos.x = 0; pos.x < lsize.x ; pos.x++){
		for(pos.y = 0; pos.y < lsize.y ; pos.y++){
			for(pos.z = 0; pos.z < lsize.z ; pos.z++){

				// check whether multiple objects claim this lattice point
				vector<Candidate> candidates;
				VDOUBLE orth_pos = lattice->to_orth(pos);
				
				for(int o = 0; o < cellobjects.size() ; o++){
					
					//cout << "cellobject " << o  << " / " << cellobjects.size() << endl;
					CellObject co = cellobjects[o];
					Candidate cand;
					cand.index = o;
					
					switch(co.type){
						
						case POINT:
							cand.distance = lattice->orth_distance( co.origin, orth_pos ); ;
							cand.abs_distance = cand.distance.abs();
							if (cand.abs_distance < 0.5) {
								candidates.push_back( cand );
							}
							break;
						case BOX:{
							// if pos is falls inside of specified box
							if( orth_pos.x >= co.origin.x && orth_pos.y >= co.origin.y && orth_pos.z >= co.origin.z 
								&& orth_pos.x <= (co.origin.x + co.boxsize.x) && orth_pos.y <= (co.origin.y + co.boxsize.y) && orth_pos.z <= (co.origin.z + co.boxsize.z) ){
								
								cand.distance = lattice->orth_distance( co.center, orth_pos );
								cand.abs_distance = cand.distance.abs();
								candidates.push_back( cand );
							}
							break;
						}
						case SPHERE:{
							cand.distance = lattice->orth_distance( co.center, orth_pos );
                            cand.abs_distance = cand.distance.abs();
							if( cand.abs_distance <= co.radius ){
								candidates.push_back( cand );
// 								cout << "Add object " << cand.index << " as candidate  for point  " << pos << endl;
							}
							break;
						}
						case ELLIPSOID:{
							//cout << pos << "\tEllipse: " << co.center << ", ax: " << co.axes << "\n";
							// if it is within the radius of the largest axes, put it as candidate
							
							if( insideEllipsoid(orth_pos, co.center, co.axes) ) {
								//cout << "INSIDE\n";

								// the shortest distance from point to center of ellipse
								//  is the distance to one of the 2 foci

								setFociEllipsoid(co);
								// 'distance' = distance to line segment between foci, weighted by distance to ellipse center
								//TODO : Missing vector distance calculation
								cand.abs_distance = distanceToLineSegment(orth_pos, co.focus1, co.focus2)
													+ lattice->orth_distance( co.center,  orth_pos ).abs();
								candidates.push_back( cand );
							}
							break;
						}
						case CYLINDER:{
							// for 3D cylinders objects, fill in the 3rd dimension
							VDOUBLE shift = (co.center2 - co.center);
							if( co.type == CYLINDER ){
								if( co.orientation == X ){
									co.center.x = pos.x;
									if(co.oblique){
										shift.x = 0.0;
										shift.y *= ((double)pos.x / (double)lsize.y);
										shift.z *= ((double)pos.x / (double)lsize.z);
										co.center += shift;
									}
								}
								else if( co.orientation == Y ){
									co.center.y = pos.y;
									if(co.oblique){
										shift.x *= ((double)pos.y / (double)lsize.x);
										shift.y = 0.0;
										shift.z *= ((double)pos.y / (double)lsize.z);
										co.center += shift;
									}
								}
								else if( co.orientation == Z ){
									co.center.z = pos.z;
									if(co.oblique){
										shift.x *= ((double)pos.z / (double)lsize.x);
										shift.y *= ((double)pos.z / (double)lsize.y);
										shift.z = 0.0;
										co.center += shift;
									}
								}
							}
							cand.distance = lattice->orth_distance( co.center, orth_pos );
                            cand.abs_distance = cand.distance.abs();
                            //cand.distance = lattice->node_distance( lattice->to_orth(VINT(co.center)), lattice->to_orth(pos) ).abs();
							if( cand.abs_distance < co.radius ){
								candidates.push_back( cand );
							}
							break;
						}
						default:{
							cerr << "InitCellObjects: Unknown type of cell object" << endl;
							exit(-1);
							break;
						}
					}
				}

				// if multiple nodes, let first one (ORDER) or closest one (DISTANCE) have the nodes
				if( candidates.size() > 0 ){
					switch( mode() ){
						
						// first one wins
						case( InitCellObjects::ORDER ):
						{  
							if( CPM::getNode(pos) == CPM::getEmptyState() ){ // do not overwrite cells (unless medium)
								CPM::setNode(pos, cellobjects[ candidates[0].index ].id );
							}
							
							break;
						}
						// closest one wins
						case( InitCellObjects::DISTANCE ): 
						{ 
							
							int winner=-1;
							double min_dist = 99999.9;
							VDOUBLE min_v_dist;
							for(int c=0; c<candidates.size();c++){
// 								if (candidates[c].abs_distance == min_dist) {
// 									VDOUBLE d_n = candidates[c].distance;
// 									VDOUBLE d_o = min_v_dist;
// 									if (( d_n.z < d_o.z || (d_n.z==d_o.z  &&  (d_n.y<d_o.y || (d_n.y==d_o.y && d_n.x<d_o.x))))) {
// 										winner = candidates[c].index;
// 										min_v_dist = d_n;
// 									}
// 								}
// 								else 
								if( candidates[c].abs_distance < min_dist ){
									min_dist = candidates[c].abs_distance;
									min_v_dist = candidates[c].distance;
									winner = candidates[c].index;
								}
							}

							// take care that nodes positions in cells are contiguous, also in case of periodic boundary conditions
// 							VINT pos_optimal = VINT(lattice->from_orth(cellobjects[winner].center)) -
// 											lattice->node_distance( VINT(lattice->from_orth(cellobjects[ winner ].center)),  VINT(pos));
							VINT pos_optimal = lattice->from_orth(cellobjects[winner].center) -
											lattice->node_distance( lattice->from_orth(cellobjects[ winner ].center),  pos);
											
// 							cout << "center: " << lattice->to_orth(cellobjects[ winner ].center)
// 								<< "\tpos:" << lattice->to_orth(pos) 
// 								<< "\tdistance: " << lattice->node_distance(  lattice->to_orth(pos), lattice->to_orth(cellobjects[ winner ].center) )
// 								<< "\tpos_c: " << lattice->to_orth(pos_optimal)
// 								<< endl;
								
							// add node to cell
// 							cout << "Adding " << pos_optimal << " to cell " << cellobjects[ winner ].id << endl;
							CPM::setNode(pos_optimal, cellobjects[ winner ].id);
							break;
						}
					}
				i++;
				}
			}
		}
	}
	return i;
}


bool more_double (const pair<double, int>& i, const pair<double, int>& j) { return (i.first > j.first); }

void InitCellObjects::setFociEllipsoid(CellObject& co){

    // sort axes lengths from large to small
    //  we keep track of the indices
    vector< std::pair<double, int> > axes(3);
    axes[0] = std::pair<double, int>(co.axes.x, 0);
    axes[1] = std::pair<double, int>(co.axes.y, 1);
    axes[2] = std::pair<double, int>(co.axes.z, 2);
    std::sort(axes.begin(), axes.end(), more_double); // axes from large to small

    // a = semimajor axis length
    double a = axes[0].first;
    // b = semiminor axis length
    double b = axes[1].first;
    // distance of focus from center
    double focus = sqrt( sqr(a) - sqr(b) );

    // from the index, we can determine the x,y,z axis along which the focus oriented
    VDOUBLE df(0.,0.,0.);
    if( axes[0].second == 0 ){ // x is semimajor
        df.x = focus;
    }
    else if( axes[0].second == 1 ){ // y is semimajor
        df.y = focus;
    }
    else if( axes[0].second == 2 ){ // z is semimajor
        df.z = focus;
    }

    // focus 1
    co.focus1 = co.center + df;
    // focus 2
    co.focus2 = co.center - df;

}


bool InitCellObjects::insideEllipsoid(VDOUBLE point, VDOUBLE center, VDOUBLE axes){
    // distance from point to center, respecting periodic boundary conditions
    VINT d = lattice->node_distance( VINT(lattice->to_orth(center)),  VINT(lattice->to_orth(point)) );

    double a  = axes.x;
    double b  = axes.y;
    double c  = axes.z;
    double p  = (sqr(d.x))/sqr(a) + (sqr(d.y))/sqr(b) + (c>0 && d.z>0 ? (sqr(d.z))/sqr(c): 0.0);
    return (p < 1);
}

double InitCellObjects::distanceToLineSegment(VDOUBLE p, VDOUBLE l1, VDOUBLE l2){

//    lattice->resolve(l1);
//    lattice->resolve(l2);

    VDOUBLE d_p_l1  = lattice->orth_distance( lattice->to_orth(p), lattice->to_orth(l1) );
    //VDOUBLE d_p_l2  = lattice->orth_distance( lattice->to_orth(p), lattice->to_orth(l2) );
    VDOUBLE d_l2_l1 = lattice->orth_distance( lattice->to_orth(l2),lattice->to_orth(l1) );

    double a = d_p_l1.x;
    double b = d_p_l1.y;
    double c = d_l2_l1.x;
    double d = d_l2_l1.y;

//    double a =  p.x - l1.x;
//    double b =  p.y - l1.y;
//    double c = l2.x - l1.x;
//    double d = l2.y - l1.y;

    double dot = a*c + b*d;
    double len_sq = sqr(c) + sqr(d);
    double param = -1;
    if(len_sq != 0)
        param = dot / len_sq;

    double xx,yy;
    if( param < 0 ){
        xx = l1.x;
        yy = l1.y;
    }
    else if( param > 1 ){
        xx = l2.x;
        yy = l2.y;
    }
    else{
        xx = l1.x + param*c;
        yy = l1.y + param*d;
    }
    VDOUBLE q(xx,yy,00);
    VDOUBLE d_p_q = lattice->orth_distance( lattice->to_orth(p), lattice->to_orth(q) );
    double dx = d_p_q.x;
    double dy = d_p_q.y;

//    double dx = p.x - xx;
//    double dy = p.y - yy;
    return sqrt( sqr(dx) + sqr(dy) );
}

