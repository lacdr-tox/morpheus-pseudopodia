#include "simulation.h"


int main(int argc, char *argv[]) {
	SIM::init(argc, argv);
	SIM::saveToXML();
	while (SIM::getMCS() < SIM::getLastMCS() ) {
		int executed = SIM::MonteCarloStep();
// 		cout << "MCS " << SIM::getMCS() << ": executed  " <<  executed << "updates\n" ;
	};
	SIM::saveToXML();
	return 0;
}
