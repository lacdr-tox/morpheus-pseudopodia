#include "add_cell.h"
#include "core/field.h"

REGISTER_PLUGIN(AddCell);

AddCell::AddCell() : InstantaneousProcessPlugin( TimeStepListener::XMLSpec::XML_OPTIONAL ) {
	
	count.setXMLPath("Count/text");
	registerPluginParameter(count);

	probdist.setXMLPath("Distribution/text");
	registerPluginParameter(probdist);

	overwrite.setXMLPath("overwrite");
	overwrite.setDefault("false");
	registerPluginParameter(overwrite);
	
	cdf = nullptr;
}

void AddCell::loadFromXML(const XMLNode xNode, Scope* scope)
{
	InstantaneousProcessPlugin::loadFromXML(xNode, scope);
	if (xNode.nChildNode("Triggers")) {
		triggers = shared_ptr<TriggeredSystem>(new TriggeredSystem);
		triggers->loadFromXML(xNode.getChildNode("Triggers"), scope);
	}
}

void AddCell::init(const Scope* scope)
{
	InstantaneousProcessPlugin::init( scope );  

	celltype = scope->getCellType();
	
	if (triggers)
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
	
	auto raw_count = count(SymbolFocus());
	// Statistically realize also dezimal counts
	int cell_count = floor(raw_count);
	cell_count += (raw_count - cell_count) > getRandom01();
	
	if( cell_count >= 1.0 ) {
		int cells_created =0;
		int iterations =0;
		int max_iterations = 10 * cell_count;
		
		if (!cdf || !probdist.flags().time_const ) {
			if (!cdf)
				cdf = new Lattice_Data_Layer<double>(SIM::getLattice(),0,0,"AddCell distribution cdf");
			double cum_sum =0;
			FocusRange range(Granularity::Node, SIM::getGlobalScope());
			for (auto focus : range) {
				auto prob =  probdist.get(focus);
				if (prob<0) {
					throw MorpheusException(string("AddCell: Distribution expression evaluates to negative probabilty at position ") + to_str(focus.pos()) + " = " + to_str(prob), this->stored_node); 
				}
				cum_sum += prob;
				cdf->set(focus.pos(), cum_sum);
			}
		}
		
		while (cell_count > cells_created && iterations<max_iterations ) {

			if (iterations >= max_iterations) {
				auto error_msg = string("AddCell: Could only place ") +to_str(cells_created) + " of " + to_str(cell_count) + " requested cells after " + to_str(max_iterations) + " tries.";
				cout << error_msg << endl;
				break;
			}
				
			iterations++;
			VINT position = this->getRandomPos();
			
			if( !overwrite() && ! checkIfMedium(position) )
					break;

			int newID = celltype->createCell();
			if (CPM::setNode(position, newID)) {
				cells_created++;
				if (triggers)
					triggers->trigger(SymbolFocus(newID));
			}
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

VINT AddCell::getRandomPos() {
	
	if (probdist.flags().space_const) {
		return CPM::getLayer()->lattice().getRandomPos();
	}

	VINT pos(0,0,0);
	// get random double between 0 and max_cdf
	const auto& cdf_data = cdf->getData();
	
	double cdf_level = getRandom01() * cdf_data[cdf_data.size()-1];
	// get index from random number of CDF
	
	auto ind = lower_bound(begin(cdf_data), end(cdf_data), cdf_level ) - begin(cdf_data);
	// get position from index
	pos = getPosFromIndex(ind, cdf->lattice().size());
	return pos;
}



