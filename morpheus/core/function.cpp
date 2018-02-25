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
	symbol.setXMLPath("symbol");
	registerPluginParameter(symbol);
	raw_expression.setXMLPath("Expression/text");
	registerPluginParameter(raw_expression);
	
	Plugin::loadFromXML(Node,scope);
	
	accessor = make_shared<Symbol>(this);
	scope->registerSymbol(accessor);
	
	evaluator = make_shared<ThreadedExpressionEvaluator<double> >(raw_expression(), false);
}

void FunctionPlugin::init (const Scope* scope) {
	Plugin::init(scope);
	evaluator->init(scope);
	accessor->setEvaluator(evaluator);
// 	registerInputSymbols( evaluator->getDependSymbols() );
// 	registerOutputSymbols(function_symbol,scope);
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
	
	Plugin::loadFromXML(Node,scope);

	description = this->plugin_name;
	
	accessor = make_shared<Symbol>(this);
	scope->registerSymbol(accessor);
	
	Plugin::loadFromXML( Node, scope );
	
	evaluator = make_shared<ThreadedExpressionEvaluator<VDOUBLE> >(raw_expression(), false);
}

void VectorFunction::init (const Scope* scope) {
	Plugin::init(scope);
	evaluator->init(scope);
	accessor->setEvaluator(evaluator);
	registerInputSymbols( evaluator->getDependSymbols() );
}
