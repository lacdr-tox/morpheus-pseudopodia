// include the header file
#include "example_plugin.h"
// macro to register plugin in framework
REGISTER_PLUGIN(ExamplePlugin);

// constructor that inherit the plugin interface. 
// XML_NONE specifies that no explicit time-step is required, i.e. plugin is scheduled automatically
ExamplePlugin::ExamplePlugin() : InstantaneousProcessPlugin( TimeStepListener::XMLSpec::XML_NONE ) {
	
	// link plugin parameter "threshold" to XML value of "threshold"
	threshold.setXMLPath("threshold");
	// set default value. Note given as a string, as it would appear in XML
	threshold.setDefault("1.0");
	registerPluginParameter(threshold);

	// link plugin parameter "expression" to XML value of "expression"
	expression.setXMLPath("expression");
	registerPluginParameter(expression);
	
};

// called before initialization
void ExamplePlugin::loadFromXML(const XMLNode xNode){
	// plugin loads parameters according to the XML paths set in constructor
	InstantaneousProcessPlugin::loadFromXML(xNode);
}

// called during initialization
void ExamplePlugin::init(const Scope* scope){
	// initialize the plugin
	InstantaneousProcessPlugin::init( scope );
	
	// get celltype from scope
	celltype = scope->getCellType();
	
	// if something goes wrong, one can throw an exception that will be shown in the message panel in the GUI.
	if( (1+1) != 2 )
		throw MorpheusException("ExamplePlugin: Serious problem.", stored_node);

}

// called periodically during simulation
void ExamplePlugin::executeTimeStep(){
	
	// get the IDs of all cells in the celltype
	vector<CPM::CELL_ID> cells = celltype->getCellIDs(); 
	
	// iterate over all cells
	for(auto& cell_id : cells){
		
		// get the focus for a particular cell (in contrast to e.g. focusing on a spatial position)
		SymbolFocus focus = SymbolFocus( cell_id );
		
		// evaluate the expression and compare the result to the (fixed) value of threshold
		// note that evaluation of expression is cell-specific, while the threshold, as a fixed value, does not need a SymbolFocus as argument 
		if( expression( focus ) > threshold( ) ){
			cout << "Expression for cell " << cell_id << " : \"" << expression.expression() << "\" =  " << expression( focus ) << " and exceeds threshold " << threshold() << endl;
			// do something useful
		}
		else
			cout << "Expression for cell " << cell_id << " : \"" << expression.expression() << "\" =  " << expression( focus ) << " and does NOT exceed threshold " << threshold() << endl;
			
	}
}