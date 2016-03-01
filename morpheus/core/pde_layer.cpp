//
// C++ Implementation: pde_layer
//
// Description:
//
//
// Author:  <>, (C) 2009
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "pde_layer.h"
#include "lattice_data_layer.cpp"
#include "expression_evaluator.h"
#include "focusrange.h"
#include <valarray>

// Explicit template instantiation
template class Lattice_Data_Layer<double>;

const float PDE_Layer::NO_VALUE = -10e6;

PDE_Layer::PDE_Layer(shared_ptr<const Lattice> l, double p_node_length, bool surface):  Lattice_Data_Layer<double>(l,1, 0.0)
{
	pde_solve_freq 		= 1;
	diffusion_rate 		= 0;
	node_length 		= p_node_length;
	wellmixed 			= false;
	store_data 			= false;
	is_surface = surface;
	
	useBuffer(true);
}


PDE_Layer::~PDE_Layer()
{/* if (write_buffer) delete[] write_buffer;*/ }

void PDE_Layer::loadFromXML(const XMLNode xNode)
{
	Lattice_Data_Layer<double>::loadFromXML(xNode,
		[] (const string& input ) -> double {
			stringstream s(input);
			value_type v;
			s >> v;
			return v;
		});
	if ( ! getXMLAttribute(xNode,"symbol",symbol_name) ) {
		cerr << "PDE_Layer needs a symbol defined.";
	}
	max_time_step = -1;

	getXMLAttribute(xNode,"Diffusion/rate", diffusion_rate);
	
	diffusion_units = "";
	getXMLAttribute(xNode,"Diffusion/unit", diffusion_units);
	if ( diffusion_units == "mm²/s" )
		diffusion_rate *= 1.0e-6;// conversion from mm^2/sec => meter^2/sec
	else if  ( diffusion_units == "µm²/s" )
		diffusion_rate *= 1.0e-12; // conversion from micron^2/sec => meter^2/sec
	else if ( ! diffusion_units.empty() && diffusion_units != "m²/s") {
		cout << "Unknown diffusion unit " << diffusion_units<< "\n";
		cout << "Valid units are m²/s, mm²/s, µm²/s"  << endl;
		cerr << "Unknown diffusion unit " << diffusion_units<< "\n";
		cerr << "Valid units are m²/s, mm²/s, µm²/s"  << endl;
	}

	wellmixed=false;
	getXMLAttribute(xNode,"Diffusion/well-mixed", wellmixed);

	if(wellmixed && diffusion_rate > 0){
		cerr << "PDE_Layer:loadFromXML: Warning: specification of diffusion rate > 0 AND well-mixed are mutually exclusive." << endl;
		exit(-1);
	}

	getXMLAttribute(xNode, "time-step", max_time_step);

	getXMLAttribute(xNode,"value", initial_expression);
	
// 	XMLNode xInit = xNode.getChildNode("Initial");
// 	if ( ! xInit.isEmpty() ) {
// 		// parse for all PDE Initializers and run them
// 		for (int i=0; i < xInit.nChildNode(); i++) {
// 			XMLNode xInitPDENode = xInit.getChildNode(i);
// 			// assume its an initilizer
// 			shared_ptr<Plugin> p = PluginFactory::CreateInstance(string(xInitPDENode.getName()));
// 			if (!p) 
// 				throw MorpheusException("Unknown Initialization Plugin.", xInitPDENode);
// 			p->loadFromXML(xInitPDENode);
// 			plugins.push_back(p);
// 		}
// 	}

	XMLNode xTiffreader = xNode.getChildNode("TIFFReader");
	if ( ! xTiffreader.isEmpty() ) {
		shared_ptr<Plugin> p = PluginFactory::CreateInstance(string(xTiffreader.getName()));
		if(!p)
			throw MorpheusException("Unknown Initialization Plugin.", xTiffreader);
		p->loadFromXML(xTiffreader);
		plugins.push_back(p);
 	}
// 		// parse for all PDE Initializers and run them
// 		for (int i=0; i < xInit.nChildNode(); i++) {
// 			XMLNode xInitPDENode = xInit.getChildNode(i);
// 			// assume its an initilizer
// 			shared_ptr<Plugin> p = PluginFactory::CreateInstance(string(xInitPDENode.getName()));
// 			if (!p) 
// 				throw MorpheusException("Unknown Initialization Plugin.", xInitPDENode);
// 			p->loadFromXML(xInitPDENode);
// 			plugins.push_back(p);
// 		}
// 	}

	XMLNode xData = xNode.getChildNode("Data");
	if ( ! xData.isEmpty()) {
		restoreData(xData);
	}

	if ( diffusion_rate>0 ) {
		// forward euler diffusion stability condition
// 		 alpha = (diffusion_rate * time_interval) / sqr(node_length)* getLattice() -> getNeighborhood(1).size();
		max_time_step = 0.75 * sqr(node_length)/(lattice().getNeighborhood(1).size() * diffusion_rate);
	}
}

void PDE_Layer::init(const Scope* scope, const SymbolFocus& focus)
{
 	theta_y.resize(l_size.y);
	phi_coarsening.resize(l_size.y);
	for (uint i=0; i<l_size.y; i++) { theta_y[i] = (double(i)+0.5)  * (M_PI/l_size.y); }
	for (uint i=0; i<l_size.y; i++) { 
		phi_coarsening[i] = pow(2,floor(log(1/sin(theta_y[i]))/log(2)));
		phi_coarsening[i] = min(phi_coarsening[i], uint(l_size.x));
	}
	phi_coarsening[0]=l_size.x;
	phi_coarsening[l_size.y-1]=l_size.x;

	if (!initial_expression.empty()) {
		ExpressionEvaluator<double> init_val(initial_expression);
		init_val.init(scope);
		if (is_surface) {
			CPM::CELL_ID cell_id = focus.cellID();
			FocusRange range(Granularity::MembraneNode, cell_id);
			for (auto focus : range) {
				this->set(focus.membrane_pos(),init_val.get(focus));
			}
		}
		else {
			FocusRange range(Granularity::Node, scope);
			for (auto focus : range) {
				this->set(focus.pos(),init_val.get(focus));
			}
		}
	}
	for (uint i=0; i<plugins.size(); i++) {
		if ( dynamic_pointer_cast<Field_Initializer>( plugins[i] ) ) {
			dynamic_pointer_cast<Field_Initializer>( plugins[i] )->run(this);
		}
	}
}

XMLNode PDE_Layer::saveToXML() const
{
	XMLNode xNode =  Lattice_Data_Layer<double>::saveToXML();
	xNode.updateName("Layer");
	xNode.addAttribute("symbol", to_cstr(symbol_name));

	int node_pos=0;
	XMLNode xDiffusion = xNode.addChild("Diffusion",FALSE,node_pos++);
	xDiffusion.addAttribute("rate", to_cstr(diffusion_rate));

	if( wellmixed )
		xDiffusion.addAttribute("well-mixed", to_cstr(true));

	if (!diffusion_units.empty())
		xDiffusion.addAttribute("unit", diffusion_units.c_str());

	xNode.addChild(storeData());
	
	return xNode;
}

XMLNode PDE_Layer::storeData(string fn) const
{
	const bool to_file = true;
	XMLNode xNode =  XMLNode::createXMLTopNode("Data");

	if (to_file) {
		string filename;
		if (fn.empty())
			filename = symbol_name + "_" + SIM::getTimeName() + ".dat";
		else 
			filename = fn;
		
		ofstream out(filename.c_str(),ios_base::trunc);
		out.setf(ios_base::scientific);
		out.precision(3);
		xNode.addAttribute("filename",filename.c_str());
		Lattice_Data_Layer< double >::storeData(out);
		out.close();
	}
	else {
		stringstream out;
		out.setf(ios_base::scientific);
		out.precision(3);
		Lattice_Data_Layer< double >::storeData(out);
		xNode.addText(out.str().c_str());
	}
	
	return xNode;
}

bool PDE_Layer::restoreData(const XMLNode node)
{
// 	try {
		string symbol;
		getXMLAttribute(node,"symbol-ref",symbol);
		
		// in case of restoring PDE data, get symbol from parent node
		if(symbol.empty())
			symbol = node.getParentNode().getAttribute("symbol");
		
		// in case of restoring MembranePropertyData, get symbol-ref from node itself
		if (symbol != symbol_name)
			throw(string("Wrong symbol name in PDE_Layer::restoreData ") + symbol + " != " + symbol_name);
		
		string filename;
		if (getXMLAttribute(node, "filename",filename)) {
			ifstream in(filename.c_str());
			if (!in.is_open())
				throw string("Unable to open file: ") + filename;
			Lattice_Data_Layer< double >::restoreData(in, [] (istream& in) -> double { double t; in >> t; return t; });
			in.close();
		} else {
			string s;
			getXMLAttribute(node,"#text",s);
			stringstream in(s);
			Lattice_Data_Layer< double >::restoreData(in, [] (istream& in) -> double { double t; in >> t; return t; });
		}
// 	}
// 	catch (string s) {
// 		cerr << s << endl;
// 		return false;
// 	}
}



// calculate the state after the time interval
void PDE_Layer::doDiffusion(double delta_t){

	if( wellmixed ){
		double average_concentration = mean();
		if (using_domain) {
			for (uint i=0; i<shadow_size_size_xyz; i++) {
				write_buffer[i] = (domain[i] == Boundary::none) ? average_concentration : domain[i];
			}
		}
		else 
			write_buffer = average_concentration;
		swapBuffer();
		return;
	}

// do the diffusion
	if (diffusion_rate != 0) {

		//cout << "BoundaryConditions:: x = " << get_boundary_type("x") << " and y = " << get_boundary_type("y") << endl;

// 		if(  lattice->getDimensions() == 2 and get_boundary_type(Lattice::b_x) == Lattice::constant && get_boundary_type(Lattice::b_y) == Lattice::constant){
// // 			cout << "running ADI ";
// 			// some algorithm to automatically refine the time steps if necessary
// 			double solved_time  = 0;
// 			double partial_delta_t = delta_t;
// 			while (solved_time < delta_t) {
// 				if (solved_time > 0.0) // ADI step depends on intermediate result
// 					swap(data, write_buffer);
// 				if ( ! solve_adi_diffusion(partial_delta_t)) {
// // 					cout << "refining" << partial_delta_t << "s" << endl;
// 					partial_delta_t  = partial_delta_t / 2;
// 				} else {
// // 					cout << "+";
// 					solved_time += partial_delta_t;
// 					partial_delta_t  = min(delta_t - solved_time, partial_delta_t*2);
// 				}
// 			}
// 		} else { // else assume periodic boundary conditions ?!?!

			// some algorithm to automatically refine the time steps if necessary
			double solved_time  = 0;
			double partial_delta_t = delta_t;

			typedef bool (PDE_Layer::*SolverMethod)(double) ; 
			SolverMethod solver;
			
			if (is_surface && dimensions==2)
				solver = & PDE_Layer::solve_fwd_euler_diffusion_spheric;
			else if (using_domain) {
				solver = & PDE_Layer::solve_fwd_euler_diffusion_generalized;
			}
			else 
				solver = & PDE_Layer::solve_fwd_euler_diffusion;
			
			while (solved_time < delta_t) {
// 				if (solved_time > 0.0) // euler step depends on intermediate result
// 					data = write_buffer;
// 					swap(data, write_buffer);
				if ((this->*solver)(partial_delta_t)) {
// 				if (solve_fwd_euler_diffusion_generalized(partial_delta_t)) {
					// Apply the discrete interpolation step be forwarding time
					// and setting the state real from the buffer.
					// Diffusion does not follow the compute/apply discrimination of other time steppers since all symbols are independent.
					// This will alse readjust all the boundary values.
					solved_time += partial_delta_t;
				} else {
					// Refine the step width in order to not break the stability criterion
					partial_delta_t  = partial_delta_t / 2;
				}
			}
// 		}
	}
}


double PDE_Layer::getMaxTimeStep() {
        return max_time_step;

};


bool PDE_Layer::solve_adi_diffusion(double time_interval)
{
	value_type alpha  = (diffusion_rate * time_interval) / sqr(node_length);
// 	cout << "alpha "  << alpha << endl;
// 	Lattice::Boundary_Type b_mx = get_boundary_type(Boundary::mx);
// 	Lattice::Boundary_Type b_my = get_boundary_type(Lattice::b_my);
// 	Lattice::Boundary_Type b_mz = get_boundary_type(Lattice::b_mz);
// 
// 	Lattice::Boundary_Type b_px = get_boundary_type(Lattice::b_px);
// 	Lattice::Boundary_Type b_py = get_boundary_type(Lattice::b_py);
// 	Lattice::Boundary_Type b_pz = get_boundary_type(Lattice::b_pz);

	// explicit in the first coordinate (y) and implicit in the second one (x)
	valarray<value_type> b(l_size.x); // independent term of the system
	valarray<value_type> r(l_size.x); // temporary storage for solver results
	valarray<value_type> d1(-alpha,l_size.x); //
	valarray<value_type> d2(1+2*alpha,l_size.x);

	// We assume row aligned arrays. If not, one could switch to VINT index(i,j,0) * offset for indexing
	for ( uint j=0; j < l_size.y; j++) {
		if (j==0) {
			if (boundary_types[Boundary::my] == Boundary::constant) {
				value_type cy = this->get(VINT(0,-1,0));
				for ( uint i=0; i< l_size.x; i++) {
					b[i] = alpha * cy+(1-2*alpha)*data[j * shadow_offset.y +i]+alpha*data[(j+1)*shadow_offset.y+i];
				}
			} else {
				for ( uint i=0; i< l_size.x; i++) {
					b[i] = (1-alpha)*data[j * shadow_offset.y +i]+alpha*data[(j+1)*shadow_offset.y+i];
				}
			}
		}
		else if (j==l_size.y-1) {
			if (boundary_types[Boundary::py] == Boundary::constant) {
				value_type cy = this->get(VINT(0, l_size.y, 0));
				for ( uint i=0; i< l_size.x; i++) {
					b[i] = alpha * data[(j-1)*shadow_offset.y + i] + (1-2*alpha)*data[j * shadow_offset.y +i] + alpha*cy;
				}
			} else {
				for ( uint i=0; i< l_size.x; i++) {
					b[i] = alpha * data[(j-1)*shadow_offset.y + i] + (1-alpha)*data[j * shadow_offset.y +i];
				}
			}
		}
		else {
			for ( uint i=0; i< l_size.x; i++) {
				b[i] = alpha * data[(j-1)*shadow_offset.y + i]+(1-2*alpha)*data[j * shadow_offset.y +i]+alpha*data[(j+1)*shadow_offset.y+i];
			}
		}
		if (b.min() < 0.0) {
			cout << "neg fwd euler in ADI: " << b.min()  << " at y=" << j << endl;
			return false;
		}
		// solving the tridiagonal system
		tridiag_solver(d1,d2,d1,b,r); // result is stored in r;
		write_buffer[slice(j * shadow_offset.y, l_size.x,1)] = r;
	}

	b.resize(l_size.y); // independent term of the system
	r.resize(l_size.y);
	d1.resize(l_size.y, -alpha);
	d2.resize(l_size.y, 1+2*alpha);
// we save another matrix if we temporarily store some values  in b_save
	valarray<value_type> b_save(l_size.y);

	// implicit in the first coordinate (y) and explicit in the second one (x)
	for (int i=0; i<l_size.x; i++) {
		if (i==0) {
			if ( boundary_types[Boundary::mx] == Boundary::constant) {
				value_type cx  = this->get(VINT(-1,0,0));
				for (uint j=0; j<l_size.y; j++) {
					b[j] = alpha*cx + (1-2*alpha)*write_buffer[j*shadow_offset.y+i] + alpha*write_buffer[j*shadow_offset.y+i+1];
				}
		} else {
				for (uint j=0; j<l_size.y; j++) {
					b[j] = (1-alpha)*write_buffer[j*shadow_offset.y+i] + alpha*write_buffer[j*shadow_offset.y+i+1];
				}
			}
		}
		else if (i==l_size.x-1) {
			if (boundary_types[Boundary::px] == Boundary::constant) {
				value_type cx  = this->get(VINT(l_size.x,0,0));
				for (uint j=0; j<l_size.y; j++) {
						b[j] = alpha*b_save[j] + (1-2*alpha)*write_buffer[j*shadow_offset.y+i] + alpha*cx;
				}
			} else {
				for (uint j=0; j<l_size.y; j++) {
					b[j] = alpha*b_save[j] + (1-alpha)*write_buffer[j*shadow_offset.y+i];
				}
			}
		}
		else {
			for (uint j=0; j<l_size.y; j++) {
				b[j] = alpha*b_save[j] + (1-2*alpha)*write_buffer[j*shadow_offset.y+i] + alpha*write_buffer[j*shadow_offset.y+i+1];
			}
		}
		if (b.min() < 0.0) {
			cout << "neg fwd euler in ADI: " << b.min()  << " at x=" << i << endl;
			return false;
		}
		tridiag_solver(d1,d2,d1,b,r); // result is stored in r;

		b_save = write_buffer[slice(i ,l_size.y, shadow_offset.y)];
		write_buffer[slice(i,l_size.y, shadow_offset.y)] = r;
	}
	return true;
}

// Set fake states at the boundaries to compute the proper fluxes in fwd euler 
// diffusion, without special treatment for the boundaries
// The boundaries have to be reset after computing the diffusion kernel
// not used by the domain constraint code ...
void PDE_Layer::set_fwd_euler_diffusion_boundaries() {
		// set no-flux boundaries to the neighboring site values
	switch ( boundary_types[Boundary::mx]) {
		case Boundary::periodic:
			data[s_xmb] = data[s_xp];
			break;
		case Boundary::noflux:
			data[s_xmb] = data[s_xm];
			break;
		case Boundary::constant: 
			data[s_xmb] = this->boundary_values[Boundary::mx];
			break;
	}
	switch (boundary_types[Boundary::px]) {
		case Boundary::periodic:
			data[s_xpb] = data[s_xm];
			break;
		case Boundary::noflux:
			data[s_xpb] = data[s_xp];
			break;
		case Boundary::constant: 
			data[s_xpb] = this->boundary_values[Boundary::px];
			break;
	}
	
	if (dimensions>=2) {
		switch (boundary_types[Boundary::my]) {
			case Boundary::periodic:
				data[s_ymb] = data[s_yp];
				break;
			case Boundary::noflux:
				data[s_ymb] = data[s_ym];
				break;
			case Boundary::constant:
				data[s_ymb] = this->boundary_values[Boundary::my];
				break;
		}
		switch (boundary_types[Boundary::py]) {
			case Boundary::periodic:
				data[s_ypb] = data[s_ym];
				break;
			case Boundary::noflux:
				data[s_ypb] = data[s_yp];
				break;
			case Boundary::constant: 
				data[s_ypb] = this->boundary_values[Boundary::py];
				break;
		}
	}
	
	if (dimensions==3) {
		switch (boundary_types[Boundary::mz]) {
			case Boundary::periodic:
				data[s_zmb] = data[s_zp];
				break;
			case Boundary::noflux:
				data[s_zmb] = data[s_zm];
				break;
			case Boundary::constant: 
				data[s_zmb] = this->boundary_values[Boundary::mz];
				break;
		}
		switch (boundary_types[Boundary::pz]) {
			case Boundary::periodic:
				data[s_zpb] = data[s_zm];
				break;
			case Boundary::noflux:
				data[s_zpb] = data[s_zp];
				break;
			case Boundary::constant: 
				data[s_zpb] =  this->boundary_values[Boundary::pz];
				break;
		}
	}
}

bool PDE_Layer::solve_fwd_euler_diffusion(double time_interval)
{
	// some attempt of normalisation ...
	double alpha = (diffusion_rate * time_interval) / sqr(node_length);
	//cout << "Euler diffusion: D =  " << diffusion_rate << "; time_interval= " <<  time_interval <<
	//		";	node_length" << node_length <<  "; alpha = "<< alpha << endl;

	vector<VINT> neighbors = _lattice->getNeighborhood(1);
	
	// numerical stability criterion
	if( alpha * neighbors.size() >= 1.0 ){
		// ht < (hx^2 / 4D) (for the 2D lattice case)
// 		cout << "diffusion step in fwd euler numerically unstable! " << alpha << endl;
// 		cout <<  " time_interval > node_length² / 2D" << endl;
// 		cout <<  time_interval << " > " << sqr(node_length) / (getLattice() -> getNeighborhood( 1 ).size() * diffusion_rate) << endl;
		return false;
	}

	set_fwd_euler_diffusion_boundaries();
	vector<double> neighbor_distance(neighbors.size());
	vector<int> neighbor_index_offst(neighbors.size());
	neighbor_index_offst.resize(neighbors.size());
	for (uint i=0; i<neighbors.size(); i++) {
		neighbor_index_offst[i] = neighbors[i] * shadow_offset;
	}
	
	if (structure == Lattice::square )  {
		for (uint y=0; y<l_size.y; y++) {
			uint row_start = get_data_index(VINT(0,y,0));
			uint row_end = row_start + l_size.x;
			double beta = (1-neighbors.size()*alpha);
			for (uint ii=row_start; ii<row_end;ii++) {
				write_buffer[ii] = data[ii]*beta + alpha * ( data[ii-1] + data[ii+1] + data[ii+shadow_size.x] + data[ii-shadow_size.x] );
			}
		}
	} 
	else if (structure == Lattice::hexagonal )  {
		for (uint y=0; y<l_size.y; y++) {
			uint row_start = get_data_index(VINT(0,y,0));
			uint row_end = row_start + l_size.x;
			double beta = (1-neighbors.size()*alpha);
			for (uint ii=row_start; ii<row_end;ii++) {
				write_buffer[ii] = data[ii]*beta + alpha * (data[ii-1] + data[ii+1] 
					+ data[ii+shadow_size.x] + data[ii+shadow_size.x-1]
					+ data[ii-shadow_size.x] + data[ii-shadow_size.x+1] );
			}
		}
	} 
	else if (structure == Lattice::linear ) {
		uint row_start = get_data_index(VINT(0,0,0));
		uint row_end = row_start + l_size.x;
		double beta = (1-neighbors.size()*alpha);
		for (uint ii=row_start; ii<row_end;ii++) {
			write_buffer[ii] = data[ii]*beta + alpha * (data[ii-1]+ data[ii+1]);
		}
	}
	else if ( structure == Lattice::cubic ) {
#pragma omp parallel for //if (l_size.z > 100) 
		for (int z=0; z<l_size.z;z++) {
			for (uint y=0; y<l_size.y; y++) {
				uint row_start = get_data_index(VINT(0,y,z));
				uint row_end = row_start + l_size.x;
				double beta = (1-6*alpha);
				for (uint ii=row_start; ii<row_end;ii++) {
					write_buffer[ii] = data[ii]*beta + alpha * (data[ii-1] + data[ii+1]
						+ data[ii+shadow_offset.y] + data[ii-shadow_offset.y]
						+ data[ii+shadow_offset.z] + data[ii-shadow_offset.z]);
				}
			}
		}
	}
	else {
		cerr << "no implementation !! " << endl; assert(0); exit(-1);
	}
	swapBuffer();
	
	return true;
}

bool PDE_Layer::solve_fwd_euler_diffusion_generalized(double time_interval)
{
	if (is_surface && dimensions==2)
		return solve_fwd_euler_diffusion_spheric(time_interval);
	

	
	// some attempt of normalisation based on homogeneaous diffusion rate and regular lattice ...
	double alpha_normal = (diffusion_rate * time_interval) / sqr(node_length);

	double alpha_total = 0;
	vector<VINT> neighbors = _lattice->getNeighborhood(1);
	valarray<double> neighbor_distance(neighbors.size());
	valarray<double> neighbor_alpha(neighbors.size());
	valarray<int> neighbor_index_offst(neighbors.size());

	for (uint i=0; i<neighbors.size(); i++) {
		neighbor_index_offst[i] = neighbors[i] * shadow_offset;
		neighbor_distance[i] = _lattice->to_orth( neighbors[i]).abs();
		neighbor_alpha[i] = alpha_normal / sqr(neighbor_distance[i]);
		alpha_total += neighbor_alpha[i];
	}
	
		// numerical stability criterion
	if ( alpha_total >= 0.8 ) {
// 		cout << "reject time step " << time_interval << endl;
		return false;
	}
	
	if (using_domain) {
// 		write_buffer = data;
// 		for (uint i=0; i<neighbors.size(); i++) {
// 			uint idx = get_data_index(VINT(0,0,0));
// 			uint stop = get_data_index(l_size - VINT(1,1,1));
// // 			valarray<bool> mask = (domain[idx] == Boundary::none) && domain.shift(neighbor_index_offst[i]) != Boundary::noflux;
// // 			write_buffer[mask] += neighbor_alpha[i] * (data.shift(neighbor_index_offst)[mask] - data[mask]);
// 			for (; idx<=stop; idx++ ) {
// 				write_buffer[idx] += ((domain[idx] != Boundary::none) || (domain[idx + neighbor_index_offst[i]]) == Boundary::noflux) ? 0.0 :
// 							neighbor_alpha[i] * (data[idx + neighbor_index_offst[i]] - data[idx]); 
// 			}
// 		}

		int idx = get_data_index(VINT(0,0,0));  /** fist lattice index **/
		int stop = get_data_index(l_size - VINT(1,1,1)); // last lattice index

		if (structure == Lattice::square )  {
			double beta = (1-alpha_total);
			for (; idx<=stop; idx++ ) {
				if (domain[idx] != Boundary::none) {
					write_buffer[idx] = data[idx];
				}
				else {
					write_buffer[idx] = data[idx]
										+ ((domain[idx - shadow_offset.x] == Boundary::noflux) ? 0.0 : ( data[idx - shadow_offset.x] - data[idx]) * neighbor_alpha[0])
										+ ((domain[idx + shadow_offset.x] == Boundary::noflux) ? 0.0 : ( data[idx + shadow_offset.x] - data[idx]) * neighbor_alpha[1])
										+ ((domain[idx - shadow_offset.y] == Boundary::noflux) ? 0.0 : ( data[idx - shadow_offset.y] - data[idx]) * neighbor_alpha[2])
										+ ((domain[idx + shadow_offset.y] == Boundary::noflux) ? 0.0 : ( data[idx + shadow_offset.y] - data[idx]) * neighbor_alpha[3]);
				}
			}
		}
		else {
			for (; idx<=stop; idx++ ) {
				write_buffer[idx] = data[idx];
				if (domain[idx] != Boundary::none) continue;
				for (uint i=0; i<neighbors.size(); i++) {
					write_buffer[idx] += (domain[idx + neighbor_index_offst[i]] == Boundary::noflux) ? 0.0 :
										(data[idx + neighbor_index_offst[i]] - data[idx]) * neighbor_alpha[i];
				}
	// 			valarray<bool> mask = domain[neighbor_index_offst+idx] != Boundary::noflux;
	// 			write_buffer[idx] += ((data[neighbor_index_offst+idx].sum() - data[idx]) );

	// 				 += ((domain[idx] != Boundary::none) || (domain[idx + neighbor_index_offst[i]]) == Boundary::noflux) ? 0.0 :
	// 							neighbor_alpha[i] * (data[idx + neighbor_index_offst[i]] - data[idx]);
			}
		}
	}
	else {
		set_fwd_euler_diffusion_boundaries();
// 		size_t idx = get_data_index(VINT(0,0,0));
// 		size_t stop = get_data_index(l_size - VINT(1,1,1));
// 		for (; idx<=stop; idx++ ) {
// 			write_buffer[idx] = data[idx];
// 			for (uint i=0; i<neighbors.size(); i++) {
// 				write_buffer[idx] += (data[idx + neighbor_index_offst[i]] - data[idx]) * neighbor_alpha[i];
// 			}
// 		}
		write_buffer = data  * (1-alpha_total);
		for (uint i=0; i<neighbors.size(); i++) {
			write_buffer += neighbor_alpha[i] * (data.shift(neighbor_index_offst[i]));
		}
	}
	
	swapBuffer();
	
	return true;
}

void PDE_Layer::updateNodeLength(double nl){
	node_length = nl;
}

bool PDE_Layer::solve_fwd_euler_diffusion_spheric(double time_interval)
{
	assert(dimensions==2);
	assert(boundary_types[Boundary::mx]==Boundary::periodic);
	assert(boundary_types[Boundary::my]==Boundary::noflux);

	// some attempt of normalisation ...
	double alpha = (diffusion_rate * time_interval) / sqr(node_length);
	double max_alpha = (diffusion_rate * time_interval) / sqr(node_length/2);
	
//	cout << "alpha = " << alpha << endl;
	
	if( max_alpha * _lattice -> getNeighborhood(1).size() >= 1.0 ){
		// ht < (hx^2 / 4D) (for the 2D lattice case)
//  		cout << "diffusion step in fwd euler spherical numerically unstable! " << alpha << endl;
//  		cout <<  " time_interval > node_length² / 2D" << endl;
//  		cout <<  time_interval << " > " << sqr(node_length) / (getLattice() -> getNeighborhood( 1 ).size() * diffusion_rate) << endl;
		return false;
	}
	
	// precalculated theta angles (angle to the x-y-plane)
	valarray<double> alpha_phi(l_size.y);        // Diffusion in phi
	valarray<double> alpha_theta_up(l_size.y);   // Diffusion flux alpha in theta upwards
	valarray<double> alpha_theta_down(l_size.y); // Diffusion flux alpha in theta downwards
	
	
	// --> TODO What is the radius of the sphere ??
	for (uint i=0; i<phi_coarsening.size(); i++) { 
		alpha_phi[i] = (diffusion_rate * time_interval) / sqr(sin(theta_y[i]) * node_length * phi_coarsening[i]);
// 		cout << "y=" << i << "; theta=" << theta_y[i] << " 
	}
	
	for (uint i=0; i<alpha_theta_up.size(); i++) {
		// Diffusion in both half spheres, including compensation for differing node volumes
		if (theta_y[i] < M_PI/2) {
			alpha_theta_up[i] =    alpha;
			alpha_theta_down[i] =  alpha * (i==0 ? 0 : sin(theta_y[i-1])/sin(theta_y[i]));
		}
		else {
			alpha_theta_up[i] =    alpha * (i==theta_y.size()-1 ? 0 : sin(theta_y[i+1])/sin(theta_y[i]));
			alpha_theta_down[i] =  alpha;
		}
	}
	
	VINT pos(0,0,0);
	
	const valarray<double>& const_data = data;
	const valarray<double>& const_buffer = write_buffer;
	
	// X COARSENING
	for (pos.y=0; pos.y<l_size.y; pos.y++) {
		// coarsening step in the data layer and phi resp. x diffusion
		if (phi_coarsening[pos.y] != 1) {
			// coarsening
			const uint row_start = get_data_index(pos);
			const uint row_end = row_start + l_size.x;
			for (uint slice_start = row_start; slice_start<row_end; slice_start += phi_coarsening[pos.y] ) {
				// this is mass conserving redistribution
				const uint length = min(phi_coarsening[pos.y], row_end-slice_start);
				data[slice(slice_start, length, 1)] = const_data[slice(slice_start, length, 1)].sum() / length;
			}
		}
	}
	
	// DIFFUSION
	set_fwd_euler_diffusion_boundaries();
// 	double sum=0;
	
	for (pos.y=0; pos.y<l_size.y; pos.y++) {
		// PHI resp. X DIFFUSION
		pos.x=0;
		const uint row_start = get_data_index(pos);
		const uint row_end = row_start + l_size.x;
		if (phi_coarsening[pos.y] == 1) {
			// diffusion
			double gamma = (1-2*alpha_phi[pos.y]);
			for (uint ii=row_start; ii<row_end;ii++) {
				write_buffer[ii] = const_data[ii]*gamma + alpha_phi[pos.y] * (const_data[ii-1]+ const_data[ii+1]);
			}
		}
		else if (phi_coarsening[pos.y] == l_size.x) {
			// diffusion
			write_buffer[slice(row_start,l_size.x,1)] = const_data[slice(row_start,l_size.x,1)];
		}
		else {
			// diffusion
			double gamma = (1-2*alpha_phi[pos.y]);
			
			for (uint slice_start = row_start; slice_start<row_end; slice_start+=phi_coarsening[pos.y] ) {
				uint length = min(phi_coarsening[pos.y], row_end-slice_start);
				write_buffer[slice(slice_start, length, 1)] = gamma * const_data[slice_start] + alpha_phi[pos.y] * (const_data[slice_start - 1] +  const_data[slice_start + length]);
			}
		}
		
		// THETA resp. Y DIFFUSION
		write_buffer[slice(row_start, l_size.x, 1)] += const_data[slice(row_start+shadow_size.x, l_size.x, 1)] * (alpha_theta_up[pos.y]);
		write_buffer[slice(row_start, l_size.x, 1)] += const_data[slice(row_start-shadow_size.x, l_size.x, 1)] * (alpha_theta_down[pos.y]);
		write_buffer[slice(row_start, l_size.x, 1)] -= const_data[slice(row_start, l_size.x, 1)]  * (alpha_theta_up[pos.y] + alpha_theta_down[pos.y]);
		
// 		sum += const_buffer[slice(row_start, l_size.x, 1)].sum() * sin(theta_y[pos.y]);
	}
	
	swapBuffer();
	
	return true;
}

void PDE_Layer::write_ascii(ostream& out) const {
	string xsep(" "), ysep("\n"), zsep("\n");

	valarray<float> out_data(l_size.x),out_data2(l_size.x);;
	VINT pos;
	bool is_hexagonal = (_lattice->getXMLName() == "hexagonal");

	for (pos.z=0; pos.z<l_size.z; pos.z++) {
		bool first_row = true;
		for (pos.y=0; pos.y<l_size.y; pos.y+=1) {
			pos.x=0;
			uint row_start = get_data_index(pos);
			uint row_end = row_start + l_size.x;

			// copy & convert the raw data
			for (uint i=row_start, m=0; i!=row_end; i++,m++) {
				out_data[m]=float(data[i]);
			}

			// shifting the data to map hex coordinate system
			if (is_hexagonal) {
				out_data = out_data.cshift(-pos.y/2);
				// add an interpolation step
				if (pos.y%2==1) {
					out_data2 = out_data.cshift(-1);
					out_data+= out_data2;
					out_data/=2;
				}
			}
			out << out_data[0];
			for (uint i = 1 ; i < out_data.size(); i+=1) {
				out << xsep << out_data[i];
			}
			out << ysep;

// 			if (first_row) {
// 				first_row = false;
// 				if ( ! (pos.y+y_iter)<l_size.y )
// 					pos.y-=y_iter;
// 			}
		}
		out << zsep;
	}
}

void PDE_Layer::write_ascii(ostream& out, float min_value, float max_value, int max_resolution) const {
	
	string xsep(" "), ysep(" "), zsep(" ");
	if (l_size.x>1) { ysep="\n"; zsep="\n"; }
	if (l_size.y>1) zsep="\n";

	int x_iter = 1;
	if (max_resolution) x_iter = max (1, l_size.x / max_resolution);
	int y_iter = 1;
	if (max_resolution) y_iter = max (1, l_size.y / max_resolution);
	valarray<float> out_data(l_size.x),out_data2(l_size.x);;
	VINT pos;
	bool is_hexagonal = (_lattice->getXMLName() == "hexagonal");
	
	for (pos.z=0; pos.z<l_size.z; pos.z++) {
		bool first_row = true;
		for (pos.y=(y_iter)/2; pos.y<l_size.y; pos.y+=y_iter) {
			pos.x=0;
			uint row_start = get_data_index(pos);
			uint row_end = row_start + l_size.x;

			// copy & convert the raw data
			for (uint i=row_start, m=0; i!=row_end; i++,m++) {
				out_data[m] = float(data[i]);
			}

			// shifting the data to map hex coordinate system
			if (is_hexagonal) {
				out_data = out_data.cshift(-pos.y/2);
				// add an interpolation step
				if (pos.y%2==1) {
					out_data2 = out_data.cshift(-1);
					out_data+= out_data2;
					out_data/=2;
				}
			}

			// Cropping data
			if (min_value != NO_VALUE) {
				float fmin_value = min_value;
				for (uint i=row_start, m=0; i!=row_end; i++,m++) {
					if (out_data[m] < fmin_value) {
						out_data[m] = fmin_value;
					}
				}
			}
			if (max_value != NO_VALUE) {
				float fmax_value = float(max_value);
				for (uint i=row_start, m=0; i!=row_end; i++,m++) {
					if (out_data[m] > fmax_value) {
						out_data[m] = fmax_value;
					}
				}
			}

			uint i = (x_iter - 1) / 2;
			out << out_data[i];
			i+= x_iter;
			for ( ; i < out_data.size(); i+=x_iter) {
				out << xsep << out_data[i];
			}
			out << ysep;
			
			if (first_row) {
				first_row = false;
				if ( ! (pos.y+y_iter)<l_size.y )
					pos.y-=y_iter;
			}
		}
		out << zsep;
	}
}

void PDE_Layer::write_binary(ostream& fout, int max_resolution) {
	// This is a pure 2D (xy plane)  implementation used for gnuplot
	// For hexagonal lattices we assume periodic boundaries and map everything to a square !!
	int x_iter = 1;
	if (max_resolution) x_iter = max(1, l_size.x / max_resolution);
	int y_iter = 1;
	if (max_resolution) y_iter = max(1, l_size.y / max_resolution);
	float k( ceil((float)l_size.x / (float)x_iter) );
	fout.write((char*) &k, sizeof(float));
	uint row_length(k);
	//printf("PDE_Layer::write_binary: max_resolution = %d (x_iter = %d\tsize = %d, k = %f)\n",max_resolution, x_iter, l_size.x, k);
	valarray<float> out_data(row_length), out_data2(row_length);
	for (k=0; k<l_size.x; k+=x_iter) fout.write((char*) &k, sizeof(float));

	bool is_hexagonal = 	(_lattice->getXMLName() == "hexagonal");
	VINT row_start(0,0,0);
	for (row_start.y=0; row_start.y<l_size.y; row_start.y++) {
		uint row_index = get_data_index(row_start);
		// value of y axis
		k = _lattice -> to_orth(row_start).y;
		fout.write((char*) &k, sizeof(float));

		// copy & convert the raw data
		for (int i=row_index, m=0; i!=row_index+l_size.x; i+=x_iter,m++) {
				out_data[m]=float(data[i]);
		}

		// shifting the data to map hex coordinate system
		if (is_hexagonal) {
				out_data = out_data.cshift(-row_start.y/2);
				// add an interpolation step
				if (row_start.y%2==1) {
						out_data2 = out_data.cshift(-1);
						out_data+= out_data2;
						out_data/=2;
				}
		}

		fout.write((char*) &(out_data[0]), sizeof(float) * out_data.size());
	}

}

void PDE_Layer::tridiag_solver(const valarray<value_type>& a, const valarray<value_type>& b, valarray<value_type> c, valarray<value_type> d,  valarray<value_type>& x)
{
// 	assert(a.size() == b.size());
// 	assert(a.size() == c.size());
// 	assert(a.size() == d.size());
// 	assert(a.size() == x.size());
// 	assert(b[0] != 0);
// 	uint n = a.size();
// 	c[0] = c[0] / b[0];
// 	d[0] = d[0] / b[0];
// 	for (int i=1; i<n; i++) {
// 		double denom = b[i]-c[i-1]*a[i];
// 		c[i] = c[i] / denom;
// 		d[i] = (d[i]-d[i-1]*a[i])/denom;
// 	}
// 	x[n-1] = d[n-1];
// 	for (int i=n-2; i>=0; i--) {
// 		x[i] = d[i] - c[i] * x[i+1];
// 	}
}

void PDE_Layer::setDiffusionRate(double diff_rate){
	diffusion_rate = diff_rate;
}

double PDE_Layer::getDiffusionRate(){
	return wellmixed ? 1 : diffusion_rate;
}

double PDE_Layer::sum() const
{
	if (using_domain) {
		return data[domain == Boundary::none].sum();
	}
	double s=0;
	for (uint z=0; z<l_size.z; z++) {
		for (uint y=0; y<l_size.y; y++) {
			uint i = get_data_index(VINT(0,0,0));
			for ( uint x=0; x<l_size.x; x++ ) {
				s+=data[i++];
			}
		}
	}
	return s;
}

double PDE_Layer::mean() const
{
	if (using_domain) {
		return sum() / (domain == Boundary::none).sum();
	}
	return sum() / (l_size.x*l_size.y*l_size.z);
}

double PDE_Layer::variance() const
{
	double average = mean();
	if (using_domain) {
		return ((data[domain == Boundary::none] - average) * (data[domain == Boundary::none] - average)).sum() / ((domain == Boundary::none).sum() - 1);
	}
	double s=0;
	for (uint z=0; z<l_size.z; z++) {
		for (uint y=0; y<l_size.y; y++) {
			uint i = get_data_index(VINT(0,0,0));
			for ( uint x=0; x<l_size.x; x++ ) {
				s+=sqr(data[i++]-average);
			}
		}
	}
	return sqrt(s)/(l_size.x*l_size.y*l_size.z-1);
}


double PDE_Layer::min_val() const {
	if (using_domain) {
		return data[domain == Boundary::none].min();
	}
	double m = data[get_data_index(VINT(0,0,0))];
	for (uint z=0; z<l_size.z; z++) {
		for (uint y=0; y<l_size.y; y++) {
			uint i = get_data_index(VINT(0,0,0));
			for ( uint x=0; x<l_size.x; x++,i++ ) {
				m = m>data[i] ? m : data[i];
			}
		}
	}
	return m;
}

double PDE_Layer::max_val() const {
	if (using_domain) {
		return data[domain == Boundary::none].max();
	}
	double m = data[get_data_index(VINT(0,0,0))];
	for (uint z=0; z<l_size.z; z++) {
		for (uint y=0; y<l_size.y; y++) {
			uint i = get_data_index(VINT(0,0,0));
			for ( uint x=0; x<l_size.x; x++,i++ ) {
				m = m<data[i] ? m : data[i];
			}
		}
	}
	return m;
}

