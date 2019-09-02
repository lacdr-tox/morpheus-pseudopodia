#include "motility_reporter.h"

REGISTER_PLUGIN(MotilityReporter);

MotilityReporter::MotilityReporter() : ReporterPlugin(TimeStepListener::XMLSpec::XML_OPTIONAL) {
	velocity.setXMLPath("Velocity/symbol-ref");
	registerPluginParameter(velocity);
	displacement.setXMLPath("Displacement/symbol-ref");
	registerPluginParameter(displacement);
	interval.setXMLPath("time-step");
	registerPluginParameter(interval);
};

void MotilityReporter::init (const Scope* scope)
{
	ReporterPlugin::init (scope);
	celltype = scope->getCellType();
	registerCellPositionDependency();

	assert(celltype);
	is_adjustable=false;

}

void MotilityReporter::report()
{
	const vector<CPM::CELL_ID> cells = celltype->getCellIDs();
	if (velocity.isDefined()) {
		for (auto cell_id : cells) {
			SymbolFocus cell_focus(cell_id);
			auto it_position = position.find(cell_id);
			if (it_position != position.end() ) {
				velocity.set(cell_focus, ( cell_focus.cell().getCenter() - it_position->second) / this->timeStep());
			}
			else {
				velocity.set(cell_focus, VDOUBLE(0,0,0));
			}
			position[cell_id] = cell_focus.cell().getCenter();
		}
	}

	if (displacement.isDefined()) {
		for (auto cell_id : cells) {
			SymbolFocus cell_focus(cell_id);
			if (origin.find(cell_id) != origin.end()) {
				VDOUBLE orientation = cell_focus.cell().getCenter() - origin[cell_id];
				displacement.set(cell_focus,  orientation);
			}
			else {
				displacement.set(cell_focus, VDOUBLE(0,0,0));
				origin[ cell_id ] = cell_focus.cell().getCenter();
			}
		}
	}
}


