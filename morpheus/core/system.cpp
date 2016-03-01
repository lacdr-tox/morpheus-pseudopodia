#include "system.h"
#include "property.h"

// Time interval scaled Noise Functions for contiuous time Processes

double getRandomNormValueSDE(double mean, double stdev, double scaling) {
        return mean + getRandomGauss(stdev) * scaling;
}

const string SystemSolver::noise_scaling_symbol = "_noise_scaling";

template <SystemType system_type>
void System<system_type>::loadFromXML(const XMLNode node)
{
	local_scope = SIM::createSubScope("System");
	SIM::enterScope(local_scope);
	
	for (auto sym : local_symbols) {
		SIM::defineSymbol( Property<double>::createVariableInstance(sym.first, sym.first) );
	}

	addLocalSymbol(SystemSolver::noise_scaling_symbol, 1 );
	
	if (system_type == CONTINUOUS_SYS) {
		string str_solver;
		if (getXMLAttribute(node,"solver",str_solver)) {
			if (str_solver =="euler" || str_solver =="1") {
				solver_method = Euler_Solver;
			}
			else if (str_solver =="heun" || str_solver =="2") {
				solver_method = Heun_Solver;
			}
			else if (str_solver =="runge-kutta" || str_solver =="4") {
				solver_method = Runge_Kutta_Solver;
			}
			else {
				cout << "Unknown solver \"" << str_solver << "\" for System!\nSelecting Runge-Kutta 4th order." << endl;
				solver_method = Runge_Kutta_Solver;
			}
		}
		else
			solver_method = Runge_Kutta_Solver;
		time_scaling = 1.0;
		getXMLAttribute(node,"time-scaling",time_scaling);
	}
	else {
		solver_method = Discrete_Solver;
		time_scaling = 1.0;
	}
	
	int nPlugins = node.nChildNode();
	for(int i=0; i<nPlugins; i++) {
		XMLNode xNode = node.getChildNode(i);
		try {
			string xml_tag_name(xNode.getName());
			shared_ptr<Plugin> p = PluginFactory::CreateInstance(xml_tag_name);
			if (! p.get()) 
				throw(string("Unknown plugin " + xml_tag_name));
			
			p->loadFromXML(xNode);
			uint n_interfaces=0;
			// Wrapper that also loads constants ...
			if ( dynamic_pointer_cast<AbstractProperty>(p) ) {
				SIM::defineSymbol(dynamic_pointer_cast<AbstractProperty>(p));
				n_interfaces++;
			}
			if ( dynamic_pointer_cast<Function>(p) ) {
				shared_ptr<SystemFunc> eqn(new SystemFunc);
				eqn->type = SystemFunc::FUN;
				eqn->expression = dynamic_pointer_cast<Function>(p)->getExpr();
				eqn->symbol_name = dynamic_pointer_cast<Function>(p)->getSymbol();
				functionals.push_back(eqn);
				functions.push_back(eqn);
// 				SIM::defineSymbol(eqn);
				n_interfaces++;
			}
			if ( dynamic_pointer_cast<DifferentialEqn>(p)) {
				shared_ptr<SystemFunc> eqn(new SystemFunc);
				eqn->type = SystemFunc::ODE;
				eqn->expression = dynamic_pointer_cast<DifferentialEqn>(p)->getExpr();
				eqn->symbol_name = dynamic_pointer_cast<DifferentialEqn>(p)->getSymbol();
				functionals.push_back(eqn);
				equations.push_back(eqn);
				n_interfaces++;
			}
			if ( dynamic_pointer_cast<Equation>(p)) {
				shared_ptr<SystemFunc> eqn(new SystemFunc);
				eqn->type = SystemFunc::EQU;
				eqn->expression = dynamic_pointer_cast<Equation>(p)->getExpr();
				eqn->symbol_name = dynamic_pointer_cast<Equation>(p)->getSymbol();
				functionals.push_back(eqn);
				equations.push_back(eqn);
				n_interfaces++;
			}
			if ( dynamic_pointer_cast<VectorEquation>(p)) {
				shared_ptr<SystemFunc> eqn(new SystemFunc);
				eqn->type = SystemFunc::VEQU;
				eqn->expression = dynamic_pointer_cast<VectorEquation>(p)->getExpr();
				eqn->symbol_name = dynamic_pointer_cast<VectorEquation>(p)->getSymbol();
				eqn->vec_spherical = dynamic_pointer_cast<VectorEquation>(p)->isSpherical();
				functionals.push_back(eqn);
				equations.push_back(eqn);
				n_interfaces++;
			}
			if (n_interfaces) {
				plugins.push_back(p);
			}
		} catch(...) {
			cerr << "Exception in ODESystem::loadFromXML "<< endl; exit(-1);
		}
	}
	SIM::leaveScope();
}

template <SystemType system_type>
void System<system_type>::addLocalSymbol(string symbol, double value) {
	
	local_symbols[symbol] = value;
	
	if (local_scope) {
		shared_ptr<Property<double> > symbol_property = Property<double>::createVariableInstance(symbol,symbol);
		SIM::enterScope(local_scope);
		SIM::defineSymbol(symbol_property);
		SIM::leaveScope();
	}
}


template <SystemType system_type>
double* System<system_type>::registerVariable(const char* name, void* instance) {
	static double buzz = numeric_limits<double>::quiet_NaN();
	System<system_type>* system = reinterpret_cast<System<system_type>*>(instance);
	if (!system->available_symbols.count(name)) {
		mu::Parser::exception_type e(mu::ecINVALID_NAME,name);
		cout << "System<>::registerVariable(): Unknown symbol " << name << endl;
		cout << "Available is ";
		copy(system->available_symbols.begin(), system->available_symbols.end(),ostream_iterator<string>(cout,","));
		cout << endl;
		throw e;
	}
	return &buzz;
}

bool less_fun_dep_level(shared_ptr<SystemFunc> a, shared_ptr <SystemFunc> b) { return a->cache_pos < b->cache_pos; }

template <SystemType system_type>
void System<system_type>::init() {
	assert(local_scope);
	SIM::enterScope(local_scope);
	
// 	celltype = ct;
	
// 	cout << "System init with " <<  omp_get_max_threads() << " threads for CellType " << (ct ? ct->getName() : " NULL " ) << endl;
	for (auto plugin : plugins) {
		if ( dynamic_pointer_cast<AbstractProperty>(plugin) ) {
			plugin->init(local_scope);
		}
	}
	///////////////////////////////////////// input 
	set<string> internal_symbols;  // collect internal symbols (anything but just referenced external symbols)
	set<string> all_symbols_used;  // collect used symbols
		// Sketch: create Parser for all equations depending on fake symbol links
	// Collect a collapsed list of required symbols (don't forget the global links of odes)
	// Obtain a context specific list of available symbols.
	set<string > all_symbols = local_scope->getAllSymbolNames<double>();
	
	available_symbols = all_symbols;
	// add all variables local to the system
	
	for (uint i=0; i<functionals.size(); i++) {
		if (functionals[i]->type == SystemFunc::FUN) {
			available_symbols.insert(functionals[i]->symbol_name);
		}
		if (functionals[i]->type == SystemFunc::VEQU) {
			internal_symbols.insert(functionals[i]->symbol_name + ".x");
			internal_symbols.insert(functionals[i]->symbol_name + ".y");
			internal_symbols.insert(functionals[i]->symbol_name + ".z");
		}
		else 
			internal_symbols.insert(functionals[i]->symbol_name);
	}

	// add solver dependend noise scaling variable
// 	local_symbols.insert(pair<string,double>(SystemSolver::noise_scaling_symbol,0));
	
	// TODO : move the noise scaling to the scope ....

	
// 	for (const auto& sym : local_symbols) {
// 		available_symbols.insert(sym.first);
// 		internal_symbols.insert(sym.first);
// 	}
	
	for (uint i=0; i<functionals.size(); i++) {
		functionals[i]->parser = createMuParserInstance();
		functionals[i]->parser->SetVarFactory(&registerVariable, (void*) this);
		
		if (functionals[i]->type == SystemFunc::ODE) {

			functionals[i]->parser->DefineFun( sym_RandomNorm,  &getRandomNormValueSDE,  false);
			// Setup proper random noise generators for gaussian noise
			size_t pos=0; bool modified_expr=false;
			while ( (pos=functionals[i]->expression.find(sym_RandomNorm,pos)) != string::npos ) {
				// is a real function call
				pos += sym_RandomNorm.size();

				if (functionals[i]->expression[pos] == '(') {
					if( this->solver_method == Runge_Kutta_Solver ){
						cerr << "System: ODE contains stochastic term, but no Runge-Kutta solver for stochastic differential equations is available (choose Euler or Heun method instead)."<< endl;
						// TODO Should throw
						exit(-1);
					}
					int level = 1;
					size_t next = pos;
					while (level > 0) {
						next = functionals[i]->expression.find_first_of("()",next+1);
						if (next == string::npos ) {
							// TODO Throw non-matching parentheses @pos
						}
						else if (functionals[i]->expression[next] == '(') {
							level++;
						}
						else if (functionals[i]->expression[next] == ')') {
							level--;
						}
					}
					functionals[i]->expression.insert(next, string(",")+SystemSolver::noise_scaling_symbol);
					modified_expr=true;
				}
			}
			if(modified_expr)
			cout << "System: Adding scaling to noise function in SystemSolver: " << functionals[i]->expression << endl;
		}

		if (functionals[i]->type == SystemFunc::ODE || functionals[i]->type == SystemFunc::EQU) {
			functionals[i]->global_symbol = local_scope->findRWSymbol<double>(functionals[i]->symbol_name);
		}
		else if (functionals[i]->type == SystemFunc::VEQU) {
			functionals[i]->v_global_symbol = local_scope->findRWSymbol<VDOUBLE>(functionals[i]->symbol_name);
		}

		try{
			functionals[i]->parser->SetExpr(functionals[i]->expression);
			// Registering used variables
			const mu::varmap_type& used_vars = functionals[i]->parser->GetUsedVar();
			for ( mu::varmap_type::const_iterator var = used_vars.begin(); var!=used_vars.end(); var++) {
				functionals[i]->dep_symbols.insert(var->first);
				all_symbols_used.insert(var->first);
			}

			if ( functionals[i]->type == SystemFunc::ODE &&
				 (functionals[i]->parser->GetUsedFun().count(sym_RandomBool) ||
				  functionals[i]->parser->GetUsedFun().count(sym_RandomGamma) ||
				  functionals[i]->parser->GetUsedFun().count(sym_RandomUni) ) )
			{
					cerr << "System: stochastic ODEs can only contain normally distributed noise \"" << sym_RandomNorm << "()\"."<< endl;
					// TODO Throw reasonable error
					exit(-1);
			}
		}
		catch (mu::Parser::exception_type &e){
			string indicator;
			if (e.GetPos()>=0 and e.GetPos()<100) {
				for(int i=0; i< e.GetPos(); i++) indicator.append(" ");
				indicator.append("^");
			}
			cerr << "System: muParser error (GetUsedVar): " << e.GetMsg() << " in \n Token\t\t: " << e.GetToken() << "\n Expression\t: " << e.GetExpr() << "\n Indicator\t: " << indicator << endl;
			exit(-1);
		}
		
	}

	// Assert the time and noise scaling symbols to be included in the cache (local time scaling depends on both).  Avoids code branching.
	all_symbols_used.insert(SymbolData::Time_symbol);
	all_symbols_used.insert(SystemSolver::noise_scaling_symbol);
	
	//////////////////////////////////////////
	//       reorder functions using their interdependency
	
// 	cout << "System::init: Reordering functions..." << endl;
	set<string> unresolved_fun_symbols;
	for (uint i=0; i<functions.size();i++) {
		unresolved_fun_symbols.insert(functions[i]->symbol_name);
		functions[i]->cache_pos=-1;
	}
	vector<string> res(unresolved_fun_symbols.size());
	
	for (uint i=0;i<functions.size();i++) {
		uint j;
		for (j=0; j<functions.size();j++) {
			if (functions[j]->cache_pos==-1) {
				if ( set_intersection(
						functions[j]->dep_symbols.begin(),functions[j]->dep_symbols.end(),
						unresolved_fun_symbols.begin(),unresolved_fun_symbols.end(), res.begin()) 
						== res.begin() ) 
				{
					unresolved_fun_symbols.erase(functions[j]->symbol_name);
					functions[j]->cache_pos = i;
					break;
				}
			}
		}
		if (j==functions.size()) {
			cerr << "System: Detected a dependency loop in functions:" << endl;
			for (j=0; j<functions.size();j++) {
				if (functions[j]->cache_pos==-1) {
					cerr << functions[j]->symbol_name  << " = " <<  functions[j]->expression << endl;
				}
			}
			exit(-1);
		}
	}
	sort(functions.begin(),functions.end(),less_fun_dep_level);
	

	//////////////////////////////////////////
	//          Checking accessibility of the contexts

	context = SymbolData::UnLinked;
	granularity = Granularity::Undef;
	uint num_assignments = 0;
	for (int i=0; i<functionals.size(); i++) {
			// Check uniform output containers for non-contexted application
		if (functionals[i]->type == SystemFunc::ODE || functionals[i]->type == SystemFunc::EQU) {
			num_assignments++;
			if (context == SymbolData::UnLinked) {
				context = functionals[i]->global_symbol.getLinkType();
			}
			else if (functionals[i]->global_symbol.getLinkType() != context) {
				throw string("System: Fatal Error! Output symbols in System of Equations represent non-uniform container types!");
			}
			
			if (granularity == Granularity::Undef) {
				granularity = functionals[i]->global_symbol.getGranularity();
			}
			else if (functionals[i]->global_symbol.getGranularity() != granularity ) {
				throw string("System: Fatal Error! Output symbols in System of Equations have non-uniform granularity!");
			}
		}
		else if (functionals[i]->type == SystemFunc::VEQU) {
			num_assignments++;
			if (context == SymbolData::UnLinked) {
				context = functionals[i]->v_global_symbol.getLinkType();
			}
			else if (functionals[i]->v_global_symbol.getLinkType() != context) {
				throw string("System: Fatal Error! Output symbols in System of Equations represent non-uniform container types!");
			}
			
			if (granularity == Granularity::Undef) {
				granularity = functionals[i]->v_global_symbol.getGranularity();
			}
			else if (functionals[i]->v_global_symbol.getGranularity() != granularity ) {
				throw string("System: Fatal Error! Output symbols in System of Equations have non-uniform granularity!");
			}
		}
	}
	
	switch (context) {
		case SymbolData::CompositeSymbolLink:
			throw string("No System methods for mixed containers ...");
			exit(-1);
		case SymbolData::PDELink:
			lattice_size = VINT(0,0,0);
			if (!equations.empty())
				lattice_size = equations[0]->global_symbol.pde_layer->getWritableSize();
			break;
		case SymbolData::CellPropertyLink:
			if (functionals.front()->type == SystemFunc::VEQU) {
				celltype = functionals.front()->v_global_symbol.getScope()->getCellType();
			}
			else {
				celltype = functionals.front()->global_symbol.getScope()->getCellType();
			}
// 			if (! celltype) { cerr << "No Celltype for CellProperty symbol in Equation System" << endl; exit(-1); }
			break;
		case SymbolData::CellMembraneLink:
// 			if (! celltype) { cerr << "No Celltype for CellMembrane symbol in Equation System" << endl; exit(-1); }
			break;
		case SymbolData::GlobalLink:
			break;
		case SymbolData::UnLinked:
			if (num_assignments == 0) {
				cout << "Ooops: No context in System" << endl;
				cout << "Ooops: Appearently you defined a System without any DiffEqn, Rule or Equation " << endl;
				break;
			}
		default:
			throw string("System: Wrong symbol type '") + SymbolData::getLinkTypeName(context) + "' in System";
			break;
	}

	/////////////////////////////////////////
	//           Creating Cache Layout

	// function symbols are internal to the solver.
	int p=0;
	// Adding external symbols first -- everything that is not another category ???
	for (set<string>::const_iterator i=all_symbols_used.begin(); i!=all_symbols_used.end(); i++ ) {
		if ( internal_symbols.count(*i) == 0 ) {
			external_symbols.push_back(SIM::findSymbol<double>(*i));
			cache_layout[*i]=p;
			p++;
// 			cout << "adding ext sym " << *i << " to the cache." << endl;
		}
// 		else
// 			cout << "not adding ext sym " << *i << " to the cache." << endl;
	}

	for (uint i=0; i<functions.size();i++) {
		if ( cache_layout.count(functions[i]->symbol_name) != 0) {
			cout << "Oops in SystemSolver::init() : Functions symbol " << functions[i]->symbol_name << " already in cache layout! " << endl;
			cerr << "Multiple definition of Function for symbol " << functions[i]->symbol_name <<  " in Function " << functions[i]->expression << " within System." << endl;
			exit(-1);
			continue;
		}
		functions[i]->cache_pos=p;
		cache_layout[functions[i]->symbol_name]=p;
		p++;
	}

	for (uint i=0; i<equations.size();i++) {
		if (equations[i]->type == SystemFunc::VEQU) {
			if ( cache_layout.count(equations[i]->symbol_name + ".x") != 0) {
				cout << "Oops in SystemSolver::init() : VectorEquation symbol " << equations[i]->symbol_name << " already in cache layout! " << endl;
				cerr << "Multiple definition of VectorRule for symbol " << equations[i]->symbol_name <<  " in VectorRule " << equations[i]->expression << "." << endl;
				exit(-1);
				continue;
			}
			equations[i]->cache_pos=p;
			cache_layout[equations[i]->symbol_name + ".x"] = p;
			cache_layout[equations[i]->symbol_name + ".y"] = p+1;
			cache_layout[equations[i]->symbol_name + ".z"] = p+2;
			p+=3;
		}
		else {
			if ( cache_layout.count(equations[i]->symbol_name) != 0) {
				cout << "Oops in SystemSolver::init() : Equation symbol " << equations[i]->symbol_name << " already in cache layout! " << endl;
				cerr << "Multiple definition of DiffEqn/Rule for symbol " << equations[i]->symbol_name <<  " in DiffEqn/Rule " << equations[i]->expression << "." << endl;
				exit(-1);
				continue;
			}
			equations[i]->cache_pos=p;
			cache_layout[equations[i]->symbol_name]=p;
			p++;
		}
	}
	
	/////////////////////////////////////////
	//           Creating Solvers	
	for (uint thread = 0; thread < omp_get_max_threads(); thread ++) {
		solvers.push_back( shared_ptr<SystemSolver>(new SystemSolver(functionals,cache_layout,solver_method) ));
	}
	SIM::leaveScope();
}

template <SystemType system_type>
void System<system_type>::computeToBuffer(const SymbolFocus& f)
{
	// The cache order is essential and fixed, so never mess around with the blocks here
	int cache_pos=0;
	SystemSolver* solver = solvers[omp_get_thread_num()].get();
	// external symbols are first in cache layout
	for (int i =0; i< external_symbols.size(); i++) {
		solver->cache[i] = external_symbols[i].get(f);
		if (external_symbols[i].getLinkType() == SymbolData::Time)
			solver->cache[i]*= time_scaling;
	}

	// initialize local copies of the equation symbols;
	for (int i =0; i<equations.size(); i++) {
		if (equations[i]->type == SystemFunc::VEQU){
			VDOUBLE val = equations[i]->v_global_symbol.get(f);
			double *v = &solver->cache[equations[i]->cache_pos];
			*v = val.x;
			*(v+1) = val.y;
			*(v+2) = val.z;
		}
		else 
			solver->cache[equations[i]->cache_pos] = equations[i]->global_symbol.get(f);
	}
	
	solver->solve();

	// update global symbols to new calculated ODE values
	for (int i =0; i<equations.size(); i++) {
		if (equations[i]->type == SystemFunc::VEQU) {
			double *v = &solver->cache[equations[i]->cache_pos];
			VDOUBLE value(*v,*(v+1),*(v+2));
			if (equations[i]->vec_spherical)
				equations[i]->v_global_symbol.setBuffer(f,VDOUBLE::from_radial(value));
			else
				equations[i]->v_global_symbol.setBuffer(f,value);
		}
		else
			equations[i]->global_symbol.setBuffer(f,solver->cache[equations[i]->cache_pos]);
	}
}

template <SystemType system_type>
void System<system_type>::applyBuffer(const SymbolFocus& f)
{
	for (int i =0; i<equations.size(); i++) {
		equations[i]->global_symbol.swapBuffer(f);
	}
}

template <SystemType system_type>
void System<system_type>::compute(const SymbolFocus& f)
{
	SystemSolver* solver = solvers[omp_get_thread_num()].get();
	for (int i =0; i< external_symbols.size(); i++) {
		solver->cache[i] = external_symbols[i].get(f);
		if (external_symbols[i].getLinkType() == SymbolData::Time)
			solver->cache[i]*= time_scaling;
	}

	// initialize local copies of the equation symbols;
	for (int i =0; i<equations.size(); i++) {
		if (equations[i]->type == SystemFunc::VEQU){
			VDOUBLE val = equations[i]->v_global_symbol.get(f);
			double *v = &solver->cache[equations[i]->cache_pos];
			*v = val.x;
			*(v+1) = val.y;
			*(v+2) = val.z;
		}
		else 
			solver->cache[equations[i]->cache_pos] = equations[i]->global_symbol.get(f);
	}
	
	solver->solve();

	// update global symbols to new calculated ODE values
	for (int i =0; i<equations.size(); i++) {
		if (equations[i]->type == SystemFunc::VEQU){
			double *v = &solver->cache[equations[i]->cache_pos];
			if ( ! equations[i]->v_global_symbol.set(f,VDOUBLE(*v,*(v+1),*(v+2))) )
				throw string("ODESystem error");
		}
		else
			if ( ! equations[i]->global_symbol.set(f,solver->cache[equations[i]->cache_pos]))
				throw string("ODESystem error");
	}
}

template <SystemType system_type>
void System<system_type>::computeContextToBuffer()
{
	// TODO This might be dispensible if the pde_layer initializes the buffer accordingly
	if (context ==  SymbolData::PDELink && SIM::lattice().getDomain().domainType()!= Domain::none) {
		for (int i =0; i<equations.size(); i++) {
			equations[i]->global_symbol.pde_layer->copyDataToBuffer();
		}
	}
	FocusRange range(granularity, local_scope);
	if (range.size() > 50) {
		int size = range.size();
#pragma omp parallel for schedule(static)
		for (auto focus = range.begin(); focus<range.end(); ++focus) {
			computeToBuffer(*focus);
		}
	}
	else {
		for (auto focus :range ) {
			computeToBuffer(focus);
		}
	}
}

template <SystemType system_type>
void System<system_type>::applyContextBuffer()
{
	for (uint i =0; i<equations.size(); i++) {
		if (equations[i]->type == SystemFunc::VEQU){
			equations[i]->v_global_symbol.swapBuffer();
		}
		else {
			equations[i]->global_symbol.swapBuffer();
		}
	}
}

template <SystemType system_type>
void System<system_type>::setTimeStep ( double ht )
{
	for (uint i=0; i<solvers.size(); i++) solvers[i]->setTimeStep(ht);
}

template <SystemType system_type>
set< SymbolDependency > System<system_type>::getDependSymbols()
{
	set<SymbolDependency> s;
	for (const auto & sym : external_symbols ) {
		SymbolDependency sd = { sym.getName(), sym.getScope() };
		s.insert(sd);
	}

	// Now remove all locally defined symbols(not registered as global symbols)
// 	for (uint i=0; i<functions.size(); i++) {
// 		SymbolDependency sd = {functions[i]->symbol_name, local_scope};
// 		s.erase(sd);
// 	}
	// Remove time && space dependencies
	SymbolDependency sd = {SymbolData::Time_symbol, SIM::getGlobalScope() };
	s.erase(sd);
	
	sd = {SystemSolver::noise_scaling_symbol, local_scope };
	s.erase(sd);
	
	sd = {SymbolData::Space_symbol, SIM::getGlobalScope() };
	s.erase(sd);

    return s;
}

template <SystemType system_type>
set< SymbolDependency > System<system_type>::getOutputSymbols()
{
	set<SymbolDependency> s;
	for (uint i=0; i<equations.size(); i++) {
		if (equations[i]->type == SystemFunc::VEQU) {
			SymbolDependency sd = { equations[i]->v_global_symbol.getName(), equations[i]->v_global_symbol.getScope() };
			s.insert(sd);
		}
		else {
			SymbolDependency sd = { equations[i]->global_symbol.getName(), equations[i]->global_symbol.getScope() };
			s.insert(sd);
		}
	}

    return s;
}



SystemSolver::SystemSolver(vector< shared_ptr< SystemFunc > > f, map< string, int > cache_layout, SolverMethod method)
{
	solver_method = method;
	this->cache_layout = cache_layout;
	cache.resize(cache_layout.size(),0.0);
	
	// Rewire the variable references to the local cache
	for (uint i=0; i<f.size(); i++) {
		if (f[i]->type == SystemFunc::FUN) {
			functions.push_back(f[i]->clone());
			assert(cache_layout.count(f[i]->symbol_name));
			functions.back().value = &cache[cache_layout[f[i]->symbol_name]];

			mu::Parser& parser = *functions.back().parser;
			mu::varmap_type used_var = parser.GetUsedVar();
			parser.ClearVar();
			for (mu::varmap_type::const_iterator var = used_var.begin(); var != used_var.end(); var++) {
				if (!cache_layout.count(var->first)) {
					cout << "Unable to find " << var->first << "in cache layout" << endl;
					cout << "Available are ";
					for (map< string, int >::const_iterator i = cache_layout.begin(); i!=cache_layout.end();i++)
						cout << i->first << ",";
					cout << endl;
				}
				assert(cache_layout.count(var->first)>0);
				parser.DefineVar(var->first, &cache[cache_layout[var->first]]);
			}
		}
		else {
			SystemFunc e = f[i]->clone();
			if (e.type == SystemFunc::VEQU) {
				assert(cache_layout.count(f[i]->symbol_name + ".x"));
				assert(cache_layout.count(f[i]->symbol_name + ".y"));
				assert(cache_layout.count(f[i]->symbol_name + ".z"));
				e.value_x = &cache[cache_layout[f[i]->symbol_name + ".x"]];
				e.value_y = &cache[cache_layout[f[i]->symbol_name + ".y"]];
				e.value_z = &cache[cache_layout[f[i]->symbol_name + ".z"]];
			}
			else {
				assert(cache_layout.count(f[i]->symbol_name));
				e.value = &cache[cache_layout[f[i]->symbol_name]];
			}

			mu::varmap_type used_var = e.parser->GetUsedVar();
			e.parser->ClearVar();
			for (auto var : used_var) {
				if (!cache_layout.count(var.first)) {
					cout << "Unable to find " << var.first << "in cache layout" << endl;
					cout << "Available are ";
					for (auto i : cache_layout) cout << i.first << ",";
					cout << endl;
				}
				e.parser->DefineVar(var.first, &cache[cache_layout[var.first]]);
			}
			if (e.type == SystemFunc::ODE)
				odes.push_back(e);
			else if (e.type == SystemFunc::EQU)
				equations.push_back(e);
			else if (e.type == SystemFunc::VEQU)
				vec_equations.push_back(e);
		}
	}
	
	// override the global time with the local scaled time
	assert(cache_layout.count(SymbolData::Time_symbol));
	local_time = &cache[cache_layout[SymbolData::Time_symbol]];
	// allow for noise scalling
	assert(cache_layout.count(SystemSolver::noise_scaling_symbol));
	noise_scaling = &cache[cache_layout[SystemSolver::noise_scaling_symbol]];
	
};

void SystemSolver::setTimeStep(double ht) {
	time_step = ht;
}

void SystemSolver::solve() {
	switch (solver_method) {
		case Euler_Solver: Euler(time_step); break;
		case Heun_Solver : Heun(time_step); break;
		case Runge_Kutta_Solver : RungeKutta(time_step); break;
		case Discrete_Solver : Discrete(); break;
	}
}



void SystemSolver::Discrete() {
	
	if (equations.size() == 0 && vec_equations.size()==0)
		return;
	
	for (uint i =0; i<functions.size(); i++) {
		*functions[i].value = functions[i].parser->Eval();
	}
	for (uint i=0; i < equations.size(); i++){
		SystemFunc &e = equations[i];
		e.k1 = e.parser->Eval();
		if ( std::isnan( e.k1 ) || std::isinf( e.k1 ) ){
			cerr << "SystemSolver returned " << (std::isnan( e.k1 ) ? "NaN" : "Inf") << " for expression '" << e.expression << "'." << endl;
			const mu::varmap_type& symbols = e.parser->GetUsedVar();
			for (auto ii  : symbols) cerr << ii.first << "=" << *(ii.second) << "; "; 
			cerr << endl;
			exit(-1);
		}
	}
	for (uint i=0; i < vec_equations.size(); i++) {
		SystemFunc &e = vec_equations[i];
		int n;
		double* results = e.parser->Eval(n);

		e.k1 = results[0];
		e.k2 = results[1];
		e.k3 = results[2];
		
		if ( std::isnan( e.k1 ) || std::isinf( e.k1 ) ||  std::isnan( e.k2 ) || std::isinf( e.k2 ) ||  std::isnan( e.k3 ) || std::isinf( e.k3 ) ){
			cerr << "SystemSolver returned NaN/Inf for expression '" << e.expression << "'." << endl;
			const mu::varmap_type& symbols = e.parser->GetUsedVar();
			for (auto ii  : symbols) cerr << ii.first << "=" << *(ii.second) << "; "; 
			cerr << endl;
			exit(-1);
		}
	}
	for (uint i=0; i < equations.size(); i++){
		*equations[i].value = equations[i].k1;
	}
	for (uint i=0; i < vec_equations.size(); i++) {
		SystemFunc &e = vec_equations[i];
		if (e.vec_spherical) {
			VDOUBLE v = VDOUBLE::from_radial(VDOUBLE(e.k1, e.k2, e.k3));
			*e.value_x = v.x;
			*e.value_y = v.y;
			*e.value_z = v.z;
		}
		else {
			*e.value_x = e.k1;
			*e.value_y = e.k2;
			*e.value_z = e.k3;
		}
	}
}

void SystemSolver::Euler(double ht) {
	
	*noise_scaling = sqrt(1.0/ht);
	
	for (uint i =0; i<functions.size(); i++) {
		*functions[i].value = functions[i].parser->Eval();
	}

	for (uint i=0; i < odes.size(); i++){
		SystemFunc &e = odes[i];
		e.k0 = *e.value;
		e.k1 = e.parser->Eval();
		if( std::isnan( e.k1 ) || std::isinf( e.k1 ) ){
			cerr << "SystemSolver returned " << (std::isnan( e.k1 ) ? "NaN" : "Inf") << " for expression '" << e.expression << "'." << endl;
			const mu::varmap_type& symbols = e.parser->GetUsedVar();
			for (auto ii  : symbols) cerr << ii.first << "=" << *(ii.second) << "; "; 
			cerr << endl;
			exit(-1);
		}
	}
	for (uint i=0; i < odes.size(); i++){
		SystemFunc &e = odes[i];
		*e.value = e.k0 + ht * e.k1;
	}
	*local_time += ht;
	this->Discrete();
}

void SystemSolver::Heun(double ht) {
	
	*noise_scaling = sqrt(2.0/ht);
	
	for (uint i =0; i<functions.size(); i++) {
		*functions[i].value = functions[i].parser->Eval();
	}

	// First Step Interpolation
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc &e = odes[i];
		e.k0 = *e.value;
		e.k1 = e.parser->Eval();

		if( std::isnan( e.k1 ) || std::isinf( e.k1 ) ){
			cerr << "SystemSolver returned " << (std::isnan( e.k1 ) ? "NaN" : "Inf") << " for expression '" << e.expression << "'." << endl;
			const mu::varmap_type& symbols = e.parser->GetUsedVar();
			for (auto ii  : symbols) cerr << ii.first << "=" << *(ii.second) << "; "; 
			cerr << endl;
			exit(-1);
		}
	}

	for(uint i=0; i < odes.size(); i++) {
		SystemFunc &e = odes[i];
		*e.value = e.k0 + 0.5 * ht * e.k1;
	}
	*local_time += ht;

	// Second Step interpolation
	for (uint i =0; i<functions.size(); i++) {
		*functions[i].value = functions[i].parser->Eval();
	}
	
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc &e = odes[i];
		e.k2 = e.parser->Eval();
		if( std::isnan( e.k2 ) || std::isinf( e.k2 ) ){
			cerr << "SystemSolver returned " << (std::isnan( e.k2 ) ? "NaN" : "Inf") << " for expression '" << e.expression << "'." << endl;
			const mu::varmap_type& symbols = e.parser->GetUsedVar();
			for (auto ii  : symbols) cerr << ii.first << "=" << *(ii.second) << "; "; 
			cerr << endl;
			exit(-1);
		}
	}
	
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc &e = odes[i];
		*e.value = e.k0 + 0.5 * ht * (e.k1 + e.k2);
	}
	
	this->Discrete();
}

void SystemSolver::RungeKutta(double ht) {
	// constants of the butcher tableau of the classical Runge-Kutta 4th order scheme
	const double a21=0.5, a31=0.0, a32=0.5, a41=0.0, a42=0.0, a43=1.0;
	const double b1=1.0/6, b2=1.0/3, b3=1.0/3, b4=1.0/6;
	const double c2=0.5, c3=0.5, c4=1.0;
	// constants of the butcher tableau of the 3/8 Runge-Kutta 4th order scheme
// 	const double a21 = 1.0/3, a31=-1.0/3, a32=1.0, a41=1.0, a42=-1.0, a43=1.0;
// 	const double b1=1.0/8, b2=3.0/8, b3=3.0/8, b4=1.0/8;
// 	const double c2=0.5, c3=0.5, c4=1.0;

	
	// First Step interpolation
	for (uint i =0; i<functions.size(); i++) {
		*functions[i].value = functions[i].parser->Eval();
	}

	for(uint i=0; i < odes.size(); i++) {
		SystemFunc &e = odes[i];
		e.k0 = *e.value;
		e.k1 = e.parser->Eval();

		if( std::isnan( e.k1 ) || std::isinf( e.k1 ) ){
			cerr << "SystemSolver returned " << (std::isnan( e.k1 ) ? "NaN" : "Inf") << " for expression '" << e.expression << "'." << endl;
			const mu::varmap_type& symbols = e.parser->GetUsedVar();
			for (auto ii  : symbols) cerr << ii.first << "=" << *(ii.second) << "; "; 
			cerr << endl;
			exit(-1);
		}
	}

	for(uint i=0; i < odes.size(); i++) {
		SystemFunc &e = odes[i];
		*e.value = e.k0 + a21 * ht * e.k1;
	}

	// Second Step interpolation
	*local_time += c2*ht;
	for (uint i =0; i<functions.size(); i++) {
		*functions[i].value = functions[i].parser->Eval();
	}
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc &e = odes[i];
		e.k2 = e.parser->Eval();
		if( std::isnan( e.k2 ) || std::isinf( e.k2 ) ){
			cerr << "SystemSolver returned " << (std::isnan( e.k2 ) ? "NaN" : "Inf") << " for expression '" << e.expression << "'." << endl;
			const mu::varmap_type& symbols = e.parser->GetUsedVar();
			for (auto ii  : symbols) cerr << ii.first << "=" << *(ii.second) << "; "; 
			cerr << endl;
			exit(-1);
		}
	}
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc &e = odes[i];
		*e.value = e.k0 + a31 * ht * e.k1 + a32 * ht * e.k2;
	}
	
	// Third Step interpolation
	*local_time += (c3-c2)*ht;
	for (uint i =0; i<functions.size(); i++) {
		*functions[i].value = functions[i].parser->Eval();
	}
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc &e = odes[i];
		e.k3 = e.parser->Eval();
		if( std::isnan( e.k3 ) || std::isinf( e.k3 ) ){
			cerr << "SystemSolver returned " << (std::isnan( e.k3 ) ? "NaN" : "Inf") << " for expression '" << e.expression << "'." << endl;
			const mu::varmap_type& symbols = e.parser->GetUsedVar();
			for (auto ii  : symbols) cerr << ii.first << "=" << *(ii.second) << "; "; 
			cerr << endl;
			exit(-1);
		}
	}
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc &e = odes[i];
		*e.value = e.type == SystemFunc::ODE ? e.k0 + a41 * ht * e.k1 + a42 * ht * e.k2 + a43 * ht * e.k3: e.k1;
	}

	// Fourth Step interpolation
	*local_time += (c4-c3)*ht;
	for (uint i =0; i<functions.size(); i++) {
		*functions[i].value = functions[i].parser->Eval();
	}
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc &e = odes[i];
		e.k4 = e.parser->Eval();
		if( std::isnan( e.k4 ) || std::isinf( e.k4 ) ){
			cerr << "SystemSolver returned " << (std::isnan( e.k4 ) ? "NaN" : "Inf") << " for expression '" << e.expression << "'." << endl;
			const mu::varmap_type& symbols = e.parser->GetUsedVar();
			for (auto ii  : symbols) cerr << ii.first << "=" << *(ii.second) << "; "; 
			cerr << endl;
			exit(-1);
		}
	}
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc &e = odes[i];
		*e.value = e.k0 + ht *( b1*e.k1 + b2*e.k2 + b3*e.k3 + b4*e.k4);
	}
	
	this->Discrete();
}


void DiscreteSystem::init(const Scope* scope)
{
	InstantaneousProcessPlugin::init(scope);
	System<DISCRETE_SYS>::init();
	registerInputSymbols(System<DISCRETE_SYS>::getDependSymbols());
	registerOutputSymbols(System<DISCRETE_SYS>::getOutputSymbols());
}


void EventSystem::loadFromXML ( const XMLNode node )
{
    TimeStepListener::loadFromXML ( node );

	// Allow manual adjustment that cannot be overridden
	if (timeStep()>0)
		this->is_adjustable = false;

	trigger_on_change = true;
	string trigger_mode ;
	if (getXMLAttribute(node,"trigger",trigger_mode) ) {
		if (trigger_mode == "on change") {
			trigger_on_change = true;
		}
		else if (trigger_mode == "when true") {
			trigger_on_change = false;
		}
		else {
			cerr << "Invalid trigger mode \"" << trigger_mode << "\" in Event." << endl;
			exit(-1);
		}
	}

	if (node.nChildNode("Condition")) {
		string expression;
		if (getXMLAttribute(node,"Condition/text",expression))
			condition = shared_ptr<ExpressionEvaluator<double> >(new ExpressionEvaluator<double>(expression));

		XMLNode xSystem = node.deepCopy();
		XMLNode xCondition = XMLNode::createXMLTopNode("Trash");
		while (xSystem.nChildNode("Condition")) {
			xCondition.addChild(xSystem.getChildNode("Condition"));
		}
		System<DISCRETE_SYS>::loadFromXML(xSystem);
	}
	else {
		System<DISCRETE_SYS>::loadFromXML(node);
	}
}

void EventSystem::init ( const Scope* scope)
{
	InstantaneousProcessPlugin::init (scope);
	System<DISCRETE_SYS>::init();
	
	if (condition)
		condition->init(scope);

	
	;
	condition->getGranularity();
	
	
	celltype = scope->getCellType();
	
	if (trigger_on_change) {
		switch (condition->getGranularity()) {
			case Granularity::Global: {
				condition_granularity = Granularity::Global;
				condition_history_val = 0;
				break;
			}
			case Granularity::Cell : {
				condition_granularity = Granularity::Cell;
				condition_history_prop = celltype->addProperty<double>("_event_history","Event History",0);
				break;
			}
			case Granularity::MembraneNode: 
			case Granularity::Node :
			default:
				throw string("Events with on-change trigger can only be used with condition depending on globals variables or cell properties.\nMissing implementation.");
				break;
		}
	}
	
	if (condition) {
		registerInputSymbols(condition->getDependSymbols());
	}
	
	registerInputSymbols(System<DISCRETE_SYS>::getDependSymbols()); // TODO Should be a non-scheduling dependency
	registerOutputSymbols(System<DISCRETE_SYS>::getOutputSymbols());
}

void EventSystem::executeTimeStep()
{
	if (trigger_on_change) {
		if (condition_granularity == Granularity::Global) {
			double cond = condition->get(SymbolFocus::global);
			double trigger = (condition_history_val <= 0.0) && cond;
			FocusRange range(granularity, scope());
			for (auto focus : range) {
				compute(focus);
			}
			condition_history_val = cond;
		}
		else if (condition_granularity == Granularity::Cell) {
			if (celltype) {
				vector<CPM::CELL_ID> cells = celltype->getCellIDs();
				for (uint c=0; c<cells.size(); c++) {
					SymbolFocus cell_focus(cells[c]);
					double cond = condition->get(cell_focus);
					bool trigger = (condition_history_prop.get(cell_focus) <= 0.0) && (cond>0.0);

					if (trigger) {
						FocusRange range(granularity, cells[c]);
						for (auto focus : range) {
							compute(focus);
						}
					}
					
					condition_history_prop.set(cell_focus,cond);
				}
			}
		}
	}
	else {
		if (condition_granularity == Granularity::Cell){
			if (celltype) {
				vector<CPM::CELL_ID> cells = celltype->getCellIDs();
				for (uint c=0; c<cells.size(); c++) {
					SymbolFocus cell_focus(cells[c]);
					double cond = condition->get(cell_focus);
					if (cond) {
						FocusRange range(granularity, cells[c]);
						for (auto focus : range) {
							compute(focus);
						}
					}
				}
			}
		}
		else {
			FocusRange range(granularity, scope());
			for (auto focus : range) {
				if ( condition->get(focus)) {
					compute(focus);
				}
			}
		}
	}
};

void ContinuousSystem::init(const Scope* scope) {
	ContinuousProcessPlugin::init(scope);
	System<CONTINUOUS_SYS>::init();
	System<CONTINUOUS_SYS>::setTimeStep(TimeStepListener::timeStep() * time_scaling);
	registerInputSymbols(System<CONTINUOUS_SYS>::getDependSymbols());
	registerOutputSymbols(System<CONTINUOUS_SYS>::getOutputSymbols());
	Plugin::local_scope = System<CONTINUOUS_SYS>::local_scope;
};


void ContinuousSystem::loadFromXML(const XMLNode node) {
	ContinuousProcessPlugin::loadFromXML(node);
	System<CONTINUOUS_SYS>::loadFromXML(node);
	// The TimeStepListener was reading the internal timeStep, thus has to be readjusted to the global time !!
	ContinuousProcessPlugin::setTimeStep(ContinuousProcessPlugin::timeStep() / time_scaling);
};



// Register the class instances with the respective XML Tags!
REGISTER_PLUGIN(ContinuousSystem);
REGISTER_PLUGIN(DiscreteSystem);
REGISTER_PLUGIN(EventSystem);


// Explicit Template Instantiation of both System Types
template class System<DISCRETE_SYS>;
template class System<CONTINUOUS_SYS>;
