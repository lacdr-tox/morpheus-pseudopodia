#include "function.h"
#include "core/expression_evaluator.h"

// shared_ptr< mu::Parser > Function::createParserInstance() {
// 	return createMuParserInstance();
// }

namespace SIM {
	
void defineSymbol(shared_ptr<Function> f) {
	SymbolData s;
	s.link=SymbolData::FunctionLink;
	s.writable = false;
	s.granularity = Granularity::Undef;
	s.type_name=TypeInfo< double >::name();
	s.name = f->getSymbol();
	s.fullname = f->getFullName();
	s.func = f;
	SIM::defineSymbol(s);
}


void defineSymbol(shared_ptr<VectorFunction> f) {
	SymbolData s;
	s.link=SymbolData::VectorFunctionLink;
	s.writable = false;
	s.granularity = Granularity::Undef;
	s.type_name=TypeInfo< VDOUBLE >::name();
	s.name = f->getSymbol();
	s.fullname = f->getFullName();
	s.vec_func = f;
	SIM::defineSymbol(s);
}

}
/*
	TODO:
	At the moment the parser gets ALL cell properties as variables.
	And these are copied given at every MCS to the function evaluator.
	To gain performance, we should only give those properties that are in the give equation or function.
	To do this, we need to parse the equation, and determine the NECESSARY properties and give only those to the parser->
*/
REGISTER_PLUGIN ( Function );

Function::Function() :  ReporterPlugin() {}

void Function::loadFromXML ( const XMLNode Node)
{
    ReporterPlugin::loadFromXML ( Node );

	if ( ! getXMLAttribute(Node,"symbol",function_symbol) )
	throw string("Function::loadFromXML: Undefined symbol name in Function ") + plugin_name;

	if ( ! Node.nChildNode("Expression")) {
		cout << "Undefined Function/Expression/text. Using deprecated Function/text.\n!!! This feature may be removed without further notice at a future date. !!! " << endl;
		getXMLAttribute(Node,"text",raw_expression);
	} else {
		getXMLAttribute(Node,"Expression/text",raw_expression);
	}
	
	if( function_symbol.empty() )
		throw  MorpheusException("Function does not have a valid symbol \""+ function_symbol +"\". ", Node);
	if( raw_expression.empty() )
		throw  MorpheusException("Function does not have a valid expression \""+ function_symbol +"\". ", Node);
		
	evaluator = shared_ptr<ThreadedExpressionEvaluator<double> >(new ThreadedExpressionEvaluator<double>(raw_expression));
	
	function_fullname = this->plugin_name;
}

void Function::init (const Scope* scope) {
	ReporterPlugin::init(scope);
	evaluator->init(scope);
	registerInputSymbols( evaluator->getDependSymbols() );
	registerOutputSymbol(function_symbol,scope);
}


XMLNode Function::saveToXML() const
{
	// Save to XML file
	XMLNode Node = Plugin::saveToXML();
	Node.addAttribute("symbol", function_symbol.c_str());
	Node.addChild("Expression").addText(raw_expression.c_str());
	return Node;
}

string  Function::getExpr() const {
// 	cout << "Function::getExpr(): " << clean_expression << endl;
	return raw_expression;
}

double Function::get( CPM::CELL_ID cell_id) const {
	return get(SymbolFocus(cell_id));
}

double Function::get(CPM::CELL_ID cell_id, const VINT& pos) const {
	return get(SymbolFocus(cell_id,pos));
}

double Function::get(const VINT& pos) const {
	return get(SymbolFocus(pos));
}

double Function::get(const SymbolFocus& focus) const {
	return evaluator->get(focus);
}

Granularity Function::getGranularity() const
{
	return evaluator->getGranularity();
}


REGISTER_PLUGIN ( VectorFunction );

VectorFunction::VectorFunction() :  ReporterPlugin() { }

void VectorFunction::loadFromXML ( const XMLNode Node)
{
	ReporterPlugin::loadFromXML( Node );
	
	getXMLAttribute(Node,"Expression/text",raw_expression);
	is_spherical = false;
	getXMLAttribute(Node,"spherical",is_spherical);
	evaluator = shared_ptr<ExpressionEvaluator<VDOUBLE> >(new ExpressionEvaluator<VDOUBLE>(raw_expression));
	
	if ( ! getXMLAttribute(Node,"symbol",function_symbol) )
		throw string("Function::loadFromXML: Undefined symbol name in Function ") + plugin_name;
	function_fullname = this->plugin_name;
}

void VectorFunction::init (const Scope* scope) {
	ReporterPlugin::init(scope);
	evaluator->init(scope);
	registerInputSymbols( evaluator->getDependSymbols() );
}

string  VectorFunction::getExpr() const {
	return raw_expression;
}

VDOUBLE VectorFunction::get(const SymbolFocus& focus) const {
	return is_spherical ? VDOUBLE::from_radial(evaluator->get(focus)) : evaluator->get(focus);
}

Granularity VectorFunction::getGranularity() const
{
	return evaluator->getGranularity();
}
