#include "function.h"
#include "core/expression_evaluator.h"

// shared_ptr< mu::Parser > Function::createParserInstance() {
// 	return createMuParserInstance();
// }
/*
	TODO:
	At the moment the parser gets ALL cell properties as variables.
	And these are copied given at every MCS to the function evaluator.
	To gain performance, we should only give those properties that are in the give equation or function.
	To do this, we need to parse the equation, and determine the NECESSARY properties and give only those to the parser->
*/
REGISTER_PLUGIN ( FunctionPlugin );

void FunctionPlugin::loadFromXML ( const XMLNode Node, Scope* scope)
{
	local_scope = scope->createSubScope("Function");
	symbol.setXMLPath("symbol");
	registerPluginParameter(symbol);
	raw_expression.setXMLPath("Expression/text");
	registerPluginParameter(raw_expression);
	for (int i=0; i<Node.nChildNode(); i++) {
		
		// Right now, the order in the XML determines the parameter syntax of the function
		auto child = Node.getChildNode(i);
		if (string(child.getName()) == "Parameter") {
			FunParameter p;
			p.type = EvaluatorVariable::DOUBLE;
			p.symbol->setXMLPath(string("Parameter[") +to_str(parameters.size()) + "]/symbol");
			p.name->setXMLPath(string("Parameter[") +to_str(parameters.size()) + "]/name");
			registerPluginParameter(p.symbol);
			registerPluginParameter(p.name);
			parameters.emplace_back(p);
		}
// 		else if (string(child.getName()) == "VectorParameter") {
// 			FunParameter p;
// 			p.type = EvaluatorVariable::VECTOR;
// 			p.symbol->setXMLPath(string("VectorParameter[") +to_str(i) + "]/symbol");
// 			p.name->setXMLPath(string("VectorParameter[") +to_str(i) + "]/name");
// 			registerPluginParameter(p.symbol);
// 			registerPluginParameter(p.name);
// 			parameters.emplace_back(p);
// 		}
	}
	
	Plugin::loadFromXML(Node,local_scope);
	
	// register the symbol in the parental scope
	symbol.init();
	accessor = make_shared<Symbol>(this);
	scope->registerSymbol(accessor);
}

void FunctionPlugin::init (const Scope* scope) {
	if (initialized) return;
	Plugin::init(local_scope);
	
	evaluator = make_shared<ThreadedExpressionEvaluator<double> >(raw_expression(), scope, false);
	// Add Parameters as local variables to the evaluators
	// we don't add them as local variables to the scope, because then they would be shared by all evaluators and may not be used concurrently
	vector<EvaluatorVariable> parameter_table;
	for ( auto p : parameters ) {
		parameter_table.push_back( {p.symbol(),p.type} );
	}
	evaluator->setLocalsTable(parameter_table);
	
	accessor->setEvaluator(evaluator);
	initialized = true;
	evaluator->init();
}


XMLNode FunctionPlugin::saveToXML() const
{
	// Save to XML file
	XMLNode Node = Plugin::saveToXML();
	Node.addAttribute("symbol", symbol().c_str());
	Node.addChild("Expression").addText(raw_expression().c_str());
	return Node;
}



REGISTER_PLUGIN ( VectorFunction );

void VectorFunction::loadFromXML ( const XMLNode Node, Scope* scope)
{
	symbol.setXMLPath("symbol");
	registerPluginParameter(symbol);
	raw_expression.setXMLPath("Expression/text");
	registerPluginParameter(raw_expression);
	is_spherical.setXMLPath("spherical");
	is_spherical.setDefault("False");
	registerPluginParameter(is_spherical);
	
	for (int i=0; i<Node.nChildNode(); i++) {
		
		// Right now, the order in the XML determines the parameter syntax of the function
		auto child = Node.getChildNode(i);
		if (string(child.getName()) == "Parameter") {
			FunParameter p;
			p.type = EvaluatorVariable::DOUBLE;
			p.symbol->setXMLPath(string("Parameter[") +to_str(i) + "]/symbol");
			p.name->setXMLPath(string("Parameter[") +to_str(i) + "]/name");
			registerPluginParameter(p.symbol);
			registerPluginParameter(p.name);
			parameters.emplace_back(p);
		}
// 		else if (string(child.getName()) == "VectorParameter") {
// 			FunParameter p;
// 			p.type = EvaluatorVariable::VECTOR;
// 			p.symbol->setXMLPath(string("VectorParameter[") +to_str(i) + "]/symbol");
// 			p.name->setXMLPath(string("VectorParameter[") +to_str(i) + "]/name");
// 			registerPluginParameter(p.symbol);
// 			registerPluginParameter(p.name);
// 			parameters.emplace_back(p);
// 		}
	}
	Plugin::loadFromXML(Node,scope);

	description = this->plugin_name;
	symbol.init();
	accessor = make_shared<Symbol>(this);
	scope->registerSymbol(accessor);
	
	
	evaluator = make_shared<ThreadedExpressionEvaluator<VDOUBLE> >(raw_expression(), scope, false);
}

void VectorFunction::init (const Scope* scope) {
	Plugin::init(scope);
	evaluator->init();
	accessor->setEvaluator(evaluator);
	registerInputSymbols( evaluator->getDependSymbols() );
}
