#include "add_cell.h"

REGISTER_PLUGIN(AddCell);

AddCell::AddCell() : InstantaneousProcessPlugin( TimeStepListener::XMLSpec::XML_OPTIONAL ) {
	
	condition.setXMLPath("Condition/text");
	registerPluginParameter(condition);

	probdist.setXMLPath("Distribution/text");
	registerPluginParameter(probdist);

	overwrite.setXMLPath("overwrite");
	overwrite.setDefault("false");
	registerPluginParameter(overwrite);

}

void AddCell::loadFromXML(const XMLNode xNode)
{
	InstantaneousProcessPlugin::loadFromXML(xNode);
	triggers = shared_ptr<TriggeredSystem>(new TriggeredSystem);
	if (xNode.nChildNode("Triggers")) {
		triggers->loadFromXML(xNode.getChildNode("Triggers"));
	}
}

void AddCell::init(const Scope* scope)
{
	InstantaneousProcessPlugin::init( scope );  

	celltype = scope->getCellType();
	cpm_layer = CPM::getLayer();
	lsize = SIM::getLattice()->size();
	triggers->init();

	//if( this->timeStep() == 0 ){
		setTimeStep( CPM::getMCSDuration() );
		is_adjustable = false;
	//}

	// because this plugin changes the cell configuration, 
	//  the cell position need to be registered as output symbol (during init())
	registerCellPositionOutput();
}

bool AddCell::checkIfMedium(VINT pos) {
	uint celltype_at_pos = SymbolFocus(pos).celltype();
	return (celltype_at_pos == CPM::getEmptyCelltypeID());
}

void AddCell::executeTimeStep() {

	if( condition( SymbolFocus() ) >= 1.0 ){

		bool createCell = false;
		VINT position = this->getRandomPos();
		
		if( !overwrite() ) {
			if( checkIfMedium(position) )
				createCell = true;
		}
		else
			createCell = true;
		
		if( createCell ){
			int newID = celltype->createCell();
			CPM::setNode(position, newID);
			//TODO: triggers->trigger(SymbolFocus(newID));
		}
	}
}

VINT AddCell::getPosFromIndex(int ind, VINT size){
	VINT pos;
	pos.z = ind / (size.x*size.y);
	pos.y = (ind - pos.z*(size.x*size.y)) / (size.x);
	pos.x = (ind - pos.y*size.x - pos.z*(size.x*size.y));
	//cout << "Index = " << ind << ", position = " << pos << endl;
	return pos;
}

VINT AddCell::getRandomPos(){
	VINT a(0,0,0);
	int latsize = (lsize.x*lsize.y*lsize.z);
	valarray<double> cdf(1.0, latsize);

	// evaluate function at every position in the lattice
	VINT pos(0,0,0);
	cdf[0] = probdist( SymbolFocus(pos) ); 
	//cout << cdf[0] << " ";

//TODO: enable multi-threading (what to do with cdf[]?)
//#pragma omp parallel for
	for(int i=1; i<cdf.size(); i++){
		VINT pos_local = getPosFromIndex(i, lsize);
		SymbolFocus sf(pos_local);
		//cout << sf.pos() << endl;
		double val = probdist( sf );
		if(val<0) {
			cerr << "AddCell: Position expression is negative for position " << pos_local << " = " << val << endl; 
			exit(-1);
		}
		cdf[i] = cdf[i-1] + val;
		//cout << pos << ":" << cdf[i] << "\n";
	}
	//cout << endl;

	// get random double between 0 and max_cdf
	double random = getRandom01() * cdf[ cdf.size() - 1];
	// get index from random number of CDF
	int ind;
	for(ind=0; ind<cdf.size(); ind++){
		if(cdf[ind] >= random)
			break;
	}
	// get position from index
	pos = getPosFromIndex(ind, lsize);
	return pos;
}



