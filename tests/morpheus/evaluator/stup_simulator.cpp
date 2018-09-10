#include "core/lattice.h"
#include "core/scope.h"

namespace SIM {
	double getTime() { return 0;};
	double getStartTime() { return 0;};
	double getStopTime() { return 1;};
	double getNodeLength() { return 1;};
	bool dependencyGraphMode() { return false; };
	shared_ptr<Lattice> getLattice() {
		static shared_ptr<Lattice> mini_lattice;
		if (!mini_lattice) {
			Lattice::LatticeDesc desc;
			desc.structure = Lattice::square;
			desc.size = VINT(2,2,1);
			desc.neighborhood_order = 1;
			desc.boundaries = { {Boundary::px,Boundary::periodic} };
			mini_lattice = shared_ptr<Lattice>(Lattice::createLattice(desc));
		}
		return mini_lattice;
	}
	
	const Lattice& lattice() {
		return *getLattice().get();
	}
	
	const Scope* current_scope = NULL;
	
	shared_ptr<Scope> global_scope = NULL;
	Scope* getGlobalScope() {
		if (!global_scope){
			global_scope = make_shared<Scope>();
			current_scope = global_scope.get();
		}
		
		return global_scope.get();
	}
	void enterScope(const Scope* scope) {
		current_scope = scope;
	}
	void leaveScope() {
		current_scope = global_scope.get();
	}
	void saveToXML() {};
	string getTimeScaleUnit() { return "atu";}
}
