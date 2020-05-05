#include "add_cell.h"
#include "core/field.h"

REGISTER_PLUGIN(AddCell);

AddCell::AddCell() : InstantaneousProcessPlugin( TimeStepListener::XMLSpec::XML_NONE ) {
	
	count.setXMLPath("Count/text");
	registerPluginParameter(count);

	probdist.setXMLPath("Distribution/text");
	registerPluginParameter(probdist);

	overwrite.setXMLPath("overwrite");
	overwrite.setDefault("false");
	registerPluginParameter(overwrite);
	
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

	// Adjust a fixed time step
	setTimeStep( CPM::getMCSDuration() );
	is_adjustable = false;

	// because this plugin changes the cell configuration, 
	// the cell position need to be registered as output symbol (during init())
	registerCellPositionOutput();
}

void AddCell::executeTimeStep() {
	
	auto raw_count = count(SymbolFocus());
	// Statistically realize also dezimal counts
	int cell_count = floor(raw_count);
	cell_count += (raw_count - cell_count) > getRandom01();
	
	if( cell_count >= 1.0 ) {

		if (cdf.size()==0 || !probdist.flags().time_const ) {
			createCDF();
		}
		
		int cells_created =0;
		int iterations =0;
		int max_iterations = 10 * cell_count;
		int current_cell_id =0;
		while (cell_count > cells_created) {

			if (iterations >= max_iterations) {
				auto error_msg = string("AddCell: Could only place ") +to_str(cells_created) + " of " + to_str(cell_count) + " requested cells after " + to_str(max_iterations) + " tries.";
				cout << error_msg << endl;
				break;
			}

			iterations++;
			VINT position = this->getRandomPos();

			SymbolFocus focus(position);
			if( focus.celltype() != CPM::getEmptyCelltypeID() ) {
				if ( !overwrite() ) {
					continue;
				}
				else if (focus.cell().nNodes() == 1) {
					continue;
				}
			}

			// For safety reasons we would retry placing a cell if setNode() fails
			if (current_cell_id==0) {
				current_cell_id = celltype->createCell();
			}

			if (CPM::setNode(position, current_cell_id)) {
				cells_created++;
				if (triggers)
					triggers->trigger(SymbolFocus(current_cell_id));
				current_cell_id = 0;
			}
		}
	}
}

void AddCell::createCDF() {
	double cum_sum =0;
	// FocusRange automatically excludes all ranges outside of the global domain
	FocusRange range(Granularity::Node, SIM::getGlobalScope());
	cdf.resize(range.size());
	int idx=0;
	for (auto focus : range) {
		auto prob =  probdist.get(focus);
		if (prob<0) {
			throw MorpheusException(string("AddCell: Distribution expression evaluates to negative probabilty at position ") + to_str(focus.pos()) + " = " + to_str(prob), this->stored_node); 
		}
		cum_sum += prob;
		cdf[idx++] = { cum_sum, focus.pos() };
	}
}


VINT AddCell::getRandomPos() {
	
	if (probdist.flags().space_const) {
		return CPM::getLayer()->lattice().getRandomPos();
	}
	
	// get random double between 0 and max_cdf
	double cdf_level = getRandom01() * cdf[cdf.size()-1].val;
	
	// get index from random number of CDF
	auto index = lower_bound(
		begin(cdf), end(cdf),
		cdf_level,
		[=](const cdf_data & a, double level) { return a.val<level;}
	);

	// get position from index
	return index->pos;
}



