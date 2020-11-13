#include "init_cell_objects.h"

REGISTER_PLUGIN(InitCellObjects);

class Point : public InitCellObjects::CellObject {
public:
	Point(XMLNode node, const Scope * scope) : displacement(0,0,0)  { p_center->setXMLPath("center");  p_center->loadFromXML(node, scope);};
	Point(const Point&) = default;
	void init() override { p_center->init(); _center = p_center->get(SymbolFocus::global) + displacement; }
	string name() const override  { return "Point"; }
	VDOUBLE center() const override { return _center; }
	unique_ptr<CellObject> clone() const override { return make_unique<Point>(*this); }
	bool inside(const VDOUBLE& pos) const override { return distance(pos)<=0.5;}
	double affinity(const VDOUBLE & pos) const override { return inside( pos); } 
	double distance(const VDOUBLE& pos) const { return SIM::lattice().orth_distance(pos,_center).abs(); } ;
	void displace(VDOUBLE distance) override { displacement +=distance; }
private:
	PluginParameter_Shared<VDOUBLE, XMLEvaluator, RequiredPolicy> p_center;
	VDOUBLE displacement;
	VDOUBLE _center;
};

class Sphere : public InitCellObjects::CellObject {
public:
	Sphere(XMLNode node, const Scope * scope) : displacement(0,0,0)  { 
		p_center->setXMLPath("center"); p_center->loadFromXML(node, scope);
		p_radius->setXMLPath("radius"); p_radius->loadFromXML(node, scope);
	};
	Sphere(const Sphere&) = default;
	void init() override { 
		p_center->init(); 
		_center = p_center->get(SymbolFocus::global) + displacement;
		p_radius->init();
		radius = p_radius->get( SIM::lattice().from_orth(_center) );
	}
	string name() const override  { return "Sphere"; }
	unique_ptr<CellObject> clone() const override { return make_unique<Sphere>(*this); }
	VDOUBLE center() const override { return _center; }
	bool inside(const VDOUBLE& pos) const override { return distance(pos)<= 0;}
	double affinity(const VDOUBLE & pos) const override {
		double dist = SIM::lattice().orth_distance(pos,_center).abs();
		return (dist<radius) ?  1 - dist/radius : 0;
	}
	double distance(const VDOUBLE& pos) const { 
		double dist = SIM::lattice().orth_distance(pos,_center).abs();
		if (dist<radius)
			return 0;
		return dist - radius; 
	};
	void displace(VDOUBLE distance) override { displacement +=distance; }
	
private:
	PluginParameter_Shared<VDOUBLE, XMLEvaluator, RequiredPolicy> p_center;
	PluginParameter_Shared<double, XMLEvaluator, RequiredPolicy> p_radius;
	VDOUBLE displacement;
	VDOUBLE _center;
	double radius;
};


class Box : public InitCellObjects::CellObject {
public:
	Box(XMLNode node, const Scope * scope) : displacement(0,0,0)  { 
		p_origin->setXMLPath("origin"); p_origin->loadFromXML(node, scope);
		p_size->setXMLPath("size"); p_size->loadFromXML(node, scope);
	};
	Box(const Box&) = default;
	void init() override { 
		p_origin->init();
		origin = p_origin->get(SymbolFocus::global) + displacement-0.5;
		p_size->init();
		size = p_size->get( SIM::lattice().from_orth(origin) );
		if (SIM::lattice().getDimensions()<=2)  size.z=1;
		if (SIM::lattice().getDimensions()<=1)  size.y=1;
		top = origin+size;
	}
	string name() const override  { return "Box"; }
	unique_ptr<CellObject> clone() const override { return make_unique<Box>(*this); }
	VDOUBLE center() const override { return origin+0.5*size; }
	bool inside(const VDOUBLE& pos) const override { 
		return pos.x >= origin.x && pos.y >= origin.y && pos.z >= origin.z &&
		pos.x <= top.x && pos.y <= top.y && pos.z <= top.z;
	}
	
	double affinity(const VDOUBLE & pos) const override {
		double d = distance(pos);
		return (d<0) ? -d : 0;
	}
	double distance(const VDOUBLE& real_pos) const {
		VDOUBLE out_distance(0,0,0);
		VDOUBLE in_distance(0,0,0);
		bool inside = false;
		
		auto pos = SIM::lattice().orth_distance(real_pos, center()) + 0.5*size;
		
		if (pos.x<0)
			out_distance.x=-pos.x;
		else if (pos.x>=size.x) 
			out_distance.x = pos.x-size.x;
		else {
			in_distance.x = min(pos.x, size.x-pos.x);
		}
		
		if (pos.y<0)
			out_distance.y=-pos.y;
		else if (pos.y>=size.y)
			out_distance.y = pos.y-size.y;
		else {
			in_distance.y = min(pos.y, size.y-pos.y);
		}

		if (pos.z<0)
			out_distance.z=-pos.z;
		else if (pos.z>=size.z)
			out_distance.z = pos.z-size.z;
		else {
			in_distance.z = min(pos.z, size.z-pos.z);
		}

		inside = (out_distance.abs_sqr() == 0);
		if (inside) {
			// Distance from the boundary
			VDOUBLE dist = (2*in_distance)/size;
			if (SIM::lattice().getDimensions()==2) 
				return -sqrt(dist.x * dist.y);
			else if (SIM::lattice().getDimensions()==1) 
				return -dist.x;
			else 
				return -sqrt(dist.x * dist.y * dist.z);
		}
		else {
			return out_distance.abs();
		}
		
	} ;
	void displace(VDOUBLE distance) override { displacement +=distance; }
	
private:
	PluginParameter_Shared<VDOUBLE, XMLEvaluator, RequiredPolicy> p_origin;
	PluginParameter_Shared<VDOUBLE, XMLEvaluator, RequiredPolicy> p_size;
	VDOUBLE displacement;
	VDOUBLE origin;
	VDOUBLE top;
	VDOUBLE size;
};

class Ellipsoid : public InitCellObjects::CellObject {
public:
	Ellipsoid(XMLNode node, const Scope * scope) : displacement(0,0,0)  { 
		p_center->setXMLPath("center"); p_center->loadFromXML(node, scope);
		p_axes->setXMLPath("axes"); p_axes->loadFromXML(node, scope);
	};
	Ellipsoid(const Ellipsoid&) = default;
	void init() override { 
		p_center->init();
		_center = p_center->get(SymbolFocus::global) + displacement;
		p_axes->init();
		axes = p_axes->get( SIM::lattice().from_orth(_center) );
		setFoci();
	}
	string name() const override  { return "Ellipsoid"; }
	unique_ptr<CellObject> clone() const override { return make_unique<Ellipsoid>(*this); }
	VDOUBLE center() const override { return _center; }
	/// distance from point to center, respecting periodic boundary conditions
	bool inside(const VDOUBLE& pos) const override {  
		auto d = SIM::lattice().orth_distance( _center,  pos);
		double p  = (sqr(d.x))/sqr(axes.x) + (sqr(d.y))/sqr(axes.y) + (axes.z>0 && abs(d.z)>0 ? (sqr(d.z))/sqr(axes.z): 0.0);
		return (p < 1);
	}
	
	double affinity(const VDOUBLE & pos) const override {
		auto d = SIM::lattice().orth_distance( _center,  pos);
		double p  = (sqr(d.x))/sqr(axes.x) + (sqr(d.y))/sqr(axes.y) + (axes.z>0 && abs(d.z)>0 ? (sqr(d.z))/sqr(axes.z): 0.0);
		return (p<1) ? 1-p : 0;
	}
	
	double distance(const VDOUBLE& real_pos) const {
		
		auto d = SIM::lattice().orth_distance( _center,  real_pos);
		double p  = (sqr(d.x))/sqr(axes.x) + (sqr(d.y))/sqr(axes.y) + (axes.z>0 && abs(d.z)>0 ? (sqr(d.z))/sqr(axes.z): 0.0);
		if (p<1) return p-1;
		else 
			return (p-1) * max(axes.x,axes.y); // TODO This is samewhat arbitrary but might work nicely.
	} ;
	void displace(VDOUBLE distance) override { displacement +=distance; }
	
private:
	PluginParameter_Shared<VDOUBLE, XMLEvaluator, RequiredPolicy> p_center;
	PluginParameter_Shared<VDOUBLE, XMLEvaluator, RequiredPolicy> p_axes;
	VDOUBLE displacement;
	VDOUBLE _center;
	VDOUBLE axes;
	VDOUBLE focus1,focus2;
	
	
	void setFoci() {
		// sort axes lengths from large to small
		//  we keep track of the indices
		vector< std::pair<double, int> > axis_length(3);
		axis_length[0] = std::pair<double, int>(axes.x, 0);
		axis_length[1] = std::pair<double, int>(axes.y, 1);
		axis_length[2] = std::pair<double, int>(axes.z, 2);
		std::sort(axis_length.begin(), axis_length.end(), [](const pair<double, int>& i, const pair<double, int>& j) {
			return (i.first > j.first); 
		}); // axes from large to small

		// a = semimajor axis length
		double a = axis_length[0].first;
		// b = semiminor axis length
		double b = axis_length[1].first;
		// distance of focus from center
		double focus = sqrt( sqr(a) - sqr(b) );

		// from the index, we can determine the x,y,z axis along which the focus oriented
		VDOUBLE df(0.,0.,0.);
		if( axis_length[0].second == 0 ){ // x is semimajor
			df.x = focus;
		}
		else if( axis_length[0].second == 1 ){ // y is semimajor
			df.y = focus;
		}
		else if( axis_length[0].second == 2 ){ // z is semimajor
			df.z = focus;
		}
		focus1 = _center + df;
		focus2 = _center - df;
	}
};

class Cylinder : public InitCellObjects::CellObject {
public:
	enum class Orientation  { X, Y, Z };

	Cylinder(XMLNode node, const Scope * scope) : displacement(0,0,0)  {
		p_origin->setXMLPath("origin"); p_origin->loadFromXML(node, scope);
		p_length->setXMLPath("length"); p_length->loadFromXML(node, scope);
		p_radius->setXMLPath("radius"); p_radius->loadFromXML(node, scope);
		p_radius2->setXMLPath("radius2"); p_radius2->loadFromXML(node, scope);
	};
	Cylinder(const Cylinder&) = default;
	void init() override { 
		p_origin->init(); 
		p_length->init();
		p_radius->init();
		origin = p_origin(SymbolFocus::global) + displacement;
		length = p_length(SIM::lattice().from_orth(origin));
		radius = p_radius(SIM::lattice().from_orth(origin));
	}
	string name() const override  { return "Cylinder"; }
	unique_ptr<CellObject> clone() const override { return make_unique<Cylinder>(*this); }
	VDOUBLE center() const override { 
		return origin + 0.5 * length;
	}
	bool inside(const VDOUBLE& pos) const override {
		auto dp = SIM::lattice().orth_distance( pos, center());
		auto d = cross(dp,length).abs() / length.abs();
		double t = dot(dp,length) / length.abs_sqr();
		return d<=radius && t>=-0.5 && t<=0.5;
	}
	double affinity(const VDOUBLE& pos) const override {
		auto dp = SIM::lattice().orth_distance( pos, center());
		auto d = cross(dp,length).abs() / length.abs();
		double t = dot(dp,length)/length.abs_sqr();
		if (d>radius || t<-0.5 || t>0.5) return 0.0;
		return 1.0-d/radius;
	};
	void displace(VDOUBLE distance) override { displacement +=distance; }
	
/*								// for 3D cylinders objects, fill in the 3rd dimension
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
*/
	
private:
	PluginParameter_Shared<VDOUBLE, XMLEvaluator, RequiredPolicy> p_origin ;
	PluginParameter_Shared<VDOUBLE, XMLEvaluator, RequiredPolicy> p_length;
	PluginParameter_Shared<double, XMLEvaluator, RequiredPolicy> p_radius;
	PluginParameter_Shared<double, XMLEvaluator, OptionalPolicy> p_radius2;
	// actual values of the cylinder instance
	VDOUBLE origin, length;
	double radius;
	VDOUBLE displacement;
};


InitCellObjects::InitCellObjects() : Population_Initializer() {
	mode.setXMLPath("mode");
	map<string, Mode> conv_map;
	conv_map["order"] = Mode::ORDER;
	conv_map["distance"] = Mode::DISTANCE;
	mode.setConversionMap(conv_map);
	mode.setDefault("distance");
	registerPluginParameter(mode);
};

void InitCellObjects::loadFromXML(const XMLNode node, Scope* scope)
{
	Population_Initializer::loadFromXML( node, scope );
	
	PluginParameter2<VDOUBLE,XMLEvaluator,RequiredPolicy>  arrange_displacement;
	arrange_displacement.setXMLPath("displacements");
	PluginParameter2<VDOUBLE,XMLEvaluator,RequiredPolicy>  arrange_repetitions;
	arrange_repetitions.setXMLPath("repetitions");
	PluginParameter2<double,XMLEvaluator,DefaultValPolicy>  arrange_random_displacement;
	arrange_random_displacement.setXMLPath("random_displacement");
	arrange_random_displacement.setDefault("0");
	
	for (int i=0; i < node.nChildNode("Arrangement"); i++) {
		XMLNode aNode = node.getChildNode("Arrangement",i);
		arrange_repetitions.loadFromXML(aNode, scope);
		arrange_repetitions.init();
		arrange_displacement.loadFromXML(aNode, scope);
		arrange_displacement.init();
		arrange_random_displacement.loadFromXML(aNode, scope);
		arrange_random_displacement.init();
		
		VINT repetitions = VINT(arrange_repetitions(SymbolFocus::global));
		repetitions.x = (repetitions.x==0 ? 1 : repetitions.x);
		repetitions.y = (repetitions.y==0 ? 1 : repetitions.y);
		repetitions.z = (repetitions.z==0 ? 1 : repetitions.z);
		VDOUBLE displacement=arrange_displacement(SymbolFocus::global);
		double random_displacement = arrange_random_displacement(SymbolFocus::global);
		
 		cout << "InitCellObjects: Arrange: displacement: " << displacement << ", repetitions: " << repetitions << endl;

		unique_ptr<CellObject> c;
		
		for (int j=0; j < aNode.nChildNode(); j++) {
			XMLNode oNode = aNode.getChildNode(j);
			string tag_name(oNode.getName());
			if (tag_name == "Point") 
				c = make_unique<Point>(oNode, scope);
			else if (tag_name == "Sphere")
				c = make_unique<Sphere>(oNode, scope);
			else if (tag_name == "Ellipsoid")
				c = make_unique<Ellipsoid>(oNode, scope);
			else if (tag_name == "Box")
				c = make_unique<Box>(oNode, scope);
			else if (tag_name == "Cylinder")
				c = make_unique<Cylinder>(oNode, scope);
			else
				throw MorpheusException(string("Unknown Object type ") + tag_name, oNode);
			
			uint num_before = cellobjects.size();
			arrangeObjectCombinatorial(std::move(c), cellobjects, displacement, repetitions, random_displacement);
			uint num_after = cellobjects.size();
 			cout << "InitCellObjects: Arranged " << (num_after-num_before) << " objects." << endl;
		}
	}
}

//============================================================================

vector<CPM::CELL_ID> InitCellObjects::run(CellType* ct)
{
	lattice = SIM::getLattice();
	for(int n = 0; n < cellobjects.size() ; n++){
		int newID = ct->createCell();
		cellobjects[n]->setCellID( newID );
// 		cout << "CellObject " << n << " = center: " << cellobjects[n]->center() << ", id " << cellobjects[n]->cellID() << endl;
	}

	int i = setNodes(ct);
	int unset =0;
	vector<CPM::CELL_ID> cells;
	for(int n = 0; n < cellobjects.size() ; n++){
		uint cellsize = ct->getCell(cellobjects[n]->cellID()).getNodes().size();
// 		cout << "CellObject " << n << " (" << ct->getName() << "), size = " << cellsize << endl;

		if( cellsize == 0 ){
			cout << "WARNING in InitCellObject: Cell " << n << " has zero size (perhaps due to overlap with other object?). Removing cell " << n << "." << endl;
			ct->removeCell( cellobjects[n]->cellID() );
			unset++;
		}
		else  {
			cells.push_back(cellobjects[n]->cellID());
		}
	}
	cout << "InitCellObject: Added " <<  cellobjects.size() - unset << " cell(s), occupying " << i << " nodes" << endl;
	return cells;
}

//============================================================================

void InitCellObjects::arrangeObjectCombinatorial( unique_ptr<CellObject> c_template, vector< unique_ptr<CellObject> >& objectlist, VDOUBLE displacement, VINT repetitions, double random_displacement )
{
	VINT r(0,0,0);
	for(r.x=0; r.x<repetitions.x; r.x++){
		for(r.y=0; r.y<repetitions.y; r.y++){
			for(r.z=0; r.z<repetitions.z; r.z++){
				
				auto n = c_template->clone();
				n->displace(r*displacement);

				if(random_displacement > 0){
					n->displace( VDOUBLE(
					            ( getRandom01()*-0.5) * random_displacement ,
					            ( SIM::lattice().getDimensions() > 1 ? (getRandom01()*-0.5) * random_displacement : 0 ), 
					            ( SIM::lattice().getDimensions() > 2 ? (getRandom01()*-0.5) * random_displacement : 0 )
					));
				}
				n->init();

				objectlist.emplace_back( std::move(n) );
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
					
					auto affinity = cellobjects[o]->affinity(orth_pos);
					if ( affinity > 0 ) {
						candidates.push_back( {o, affinity} );
					}
				}

				// if multiple nodes, let first one (ORDER) or closest one (DISTANCE) have the nodes
				if( candidates.size() > 0 ){
					
					int winner_candidate=-1;

					switch( mode() ){
						case( Mode::ORDER ): // first one wins
						{  
							winner_candidate=0;
							break;
						}
						case( Mode::DISTANCE ): // smallest distance wins
						{ 
							double max_affinity = 0;
							for(int c=0; c<candidates.size();c++){
								if( candidates[c].affinity > max_affinity ){
									max_affinity = candidates[c].affinity;
									winner_candidate = c;
								}
							}
							break;
						}
						
					}
					
					if (winner_candidate==-1) winner_candidate=0;
					int winner_object_id = candidates[winner_candidate].index;
					
					if( CPM::getNode(pos) == CPM::getEmptyState() ) { // do not overwrite cells (unless medium)
						// take care that node positions in cells are contiguous, also in case of periodic boundary conditions
						VINT latt_center = lattice->from_orth(cellobjects[ winner_object_id ]->center());
						VINT pos_optimal = latt_center - lattice->node_distance( latt_center,  pos);
						CPM::setNode(pos_optimal, cellobjects[ winner_object_id ]->cellID() );
					}
				i++;
				}
			}
		}
	}
	return i;
}


VDOUBLE InitCellObjects::distanceToLineSegment(VDOUBLE p, VDOUBLE l1, VDOUBLE l2)
{
	// the line segment is given by 
	// l1 + t*(l2-l1), restricted to  0<=t<=1
	
	VDOUBLE p_l1  = lattice->orth_distance( lattice->to_orth(p), lattice->to_orth(l1) );
	VDOUBLE l2_l1 = lattice->orth_distance( lattice->to_orth(l2),lattice->to_orth(l1) );

	double len_sq = dot(l2_l1,l2_l1); 
	double param = -1;
	if(len_sq != 0)
		param = dot(p_l1,l2_l1) / len_sq;

	VDOUBLE line_pos;;
	if( param < 0 ){
		line_pos = l1;
	}
	else if( param > 1 ){
		line_pos = l2;
	}
	else{
		line_pos = l1 + param*l2_l1;
	}
	VDOUBLE dist = lattice->orth_distance( p, line_pos );
	return dist;
}

// double min_elip_dist(VDOUBLE y, VDOUBLE elips) {
// 	double epst = 1e-3;
// 	double x0=0, x1=0.1;
// 	
// 	while (abs(x1-x0) > epst) {
// 		x0=x1;
// 		x1=x0-
// 	}
// }

// function [z,dist, lambda] = min_elp_dist(y,vec)
// epst = 1e-10;
// x0=0;
// x1=0.1;
// while abs(x1-x0)>epst
// x0 = x1;
// x1 = x0-fun(x0,y,vec)/der(x0,y,vec);
// end
// z = (eye(length(vec))+lam*diag(vec))\y;
// dist= norm(y-z);
//  
// function res = fun(lam,y,vec);
// res = sum(vec.*(y./(lam*vec+1)).^2)-1;
//  
// function res2 = der(lam,y,vec);
// res2 = -2*sum((vec.^2).*(y.^2)./((lam*vec+1).^3));
