#include "property.h"
#include "expression_evaluator.h"

namespace SIM {
	void defineSymbol(shared_ptr<AbstractProperty> property) {
		// registering the property as a Symbol
		SymbolData symbol;
		symbol.type_name = property->getTypeName();
		symbol.fullname = property->getName();
		symbol.name = property->getSymbol();
		symbol.integer = false;
		symbol.is_delayed = property->isDelayed();
		if (property->isCellProperty()) {
			symbol.link = SymbolData::CellPropertyLink;
			symbol.granularity = Granularity::Cell;
			
			symbol.writable = true;
			symbol.time_invariant = false;
			symbol.invariant = false;
		}
		else {
			symbol.const_prop = property;
			symbol.link = (SymbolData::GlobalLink);
			// granularity is set to undef until it's initialized
			symbol.granularity = Granularity::Undef;
			
			symbol.writable = ! property->isConstant();
			symbol.invariant = property->isConstant();
			symbol.time_invariant = property->isConstant();
		}
		SIM::defineSymbol(symbol);
	}
}

template <> const string Property<double>::property_xml_name() { return "Property";};
template <> const string Property<double>::global_xml_name() { return "Variable";};
template <> const string Property<double>::constant_xml_name() { return "Constant";};

template <> const string Property<vector<double> >::property_xml_name() { return "PropertyArray";};
template <> const string Property<vector<double> >::global_xml_name() { return "VariableArray";};
template <> const string Property<vector<double> >::constant_xml_name() { return "ConstantArray";};

template <> const string Property<VDOUBLE>::property_xml_name() { return "PropertyVector";};
template <> const string Property<VDOUBLE>::global_xml_name() { return "VariableVector";};
template <> const string Property<VDOUBLE>::constant_xml_name() { return "ConstantVector";};


template <>
void Property<double>::init(const Scope* scope, const SymbolFocus& f) {
	if (initialized) return;
	AbstractProperty::init(scope, f);
	
	value = 0;
	initialized = true;
	
	auto overrides = scope->valueOverrides();
	auto it = overrides.find(this->symbolic_name);
	if ( it != overrides.end()) {
		string_val = it->second;
		scope->removeValueOverride(it->second);
	}
	
	if (!string_val.empty()) {
		ExpressionEvaluator<double> eval(string_val);
		eval.init(scope);
		value = eval.get(f);
	}
}

template <>
void Property<VDOUBLE>::init(const Scope* scope, const SymbolFocus& f) {
	if (initialized) return;
	AbstractProperty::init(scope, f); 
	
	auto overrides = scope->valueOverrides();
	auto it = overrides.find(this->symbolic_name);
	if ( it != overrides.end()) {
		string_val = it->second;
		scope->removeValueOverride(it->second);
	}
	
	if (!string_val.empty()) {
		ExpressionEvaluator<VDOUBLE> eval(string_val);
		eval.init(scope);
		value = eval.get(f);
	}
	initialized = true; 
}

DelayProperty::DelayProperty(bool cellproperty):
  Property<double>("","",false, cellproperty,true),
  ContinuousProcessPlugin(ContinuousProcessPlugin::DELAY, XMLSpec::XML_NONE),
  tsl_initialized(false)
{ };

DelayProperty::DelayProperty(string name, string symbol, bool cellproperty) :
  Property<double>(name, symbol, false, cellproperty,true),
  ContinuousProcessPlugin(ContinuousProcessPlugin::DELAY, XMLSpec::XML_NONE),
  tsl_initialized(false)
{ };

bool DelayProperty::type_registration = PluginFactory::RegisterCreatorFunction( DelayProperty::global_xml_name(), DelayProperty::createVariableInstance)
                                   && PluginFactory::RegisterCreatorFunction( DelayProperty::property_xml_name(), DelayProperty::createPropertyInstance);

shared_ptr< AbstractProperty > DelayProperty::clone() const
{
	 shared_ptr<DelayProperty> c(new  DelayProperty(*this));
	 // We Remember all clones that are attached to cells, such that we can also propagate those during executeTimeStep()
	 clones.insert(c);
	 return dynamic_pointer_cast<AbstractProperty>(c);
}

Plugin* DelayProperty::createVariableInstance() { return new DelayProperty() ; }
Plugin* DelayProperty::createPropertyInstance() { return new DelayProperty(true); }

void DelayProperty::loadFromXML(XMLNode node)
{
    Property< double >::loadFromXML(node);
	ContinuousProcessPlugin::loadFromXML(node);
	
	if (!getXMLAttribute(node,"delay",delay) ) {
		delay =0;
	}
	
	if (!type_registration)
		cout << "Don't ever remove me! " << " I take care to register this Plugin !!" << endl;
}

void DelayProperty::init(const Scope* scope, const SymbolFocus& f)
{
	cout << "Initializing DelayProperty " << symbolic_name << endl;;
	Property::init(scope, f);
	setTimeStep(delay);
	
	if ( ! tsl_initialized ) {
		ContinuousProcessPlugin::init(scope);
		registerInputSymbol(this->getSymbol(), scope);
		registerOutputSymbol(this->getSymbol(), scope);
		tsl_initialized = true;
	}
}


void DelayProperty::setTimeStep(double t)
{
	ContinuousProcessPlugin::setTimeStep(t);
	int queue_length;
	if (t==0) {
		queue_length = 1;
	}
	else {
		assert(t<=delay);
		if ( abs(delay/t - rint(delay/t)) > 0.01 ) {
			throw string("Time Stepping override (") + to_str(t) + ") for DelayProperty " +  symbolic_name + " is not an integer fraction of the time delay ("  + to_str(delay)  + ").";
			exit(-1);
		}
		queue_length = max(int(rint(delay/t)),1);
		queue_length += 1; // need one more storage locations than intervals;
	}
	queue.resize(queue_length,value);
	std::set< shared_ptr<DelayProperty> >::iterator i;
	//  Propagate all cloned containers attached to cells
	for (i=clones.begin(); i!=clones.end();i++) {
		(*i)->queue.resize(queue.size(),value);
	}
	cout << "Queue for DelayProperty " << name << " resized to length " << queue.size() << " with step size " << timeStep() << endl;
}

void DelayProperty::executeTimeStep()
{
	if (this->isCellProperty()) {
		//  Execute time step for all cloned containers attached to cells
		for (const auto& clone : clones) {
			clone->queue.push_back(clone->queue.back());
			clone->queue.pop_front();
			clone->value = clone->queue.front();
		}
	}
	else {
		queue.push_back(queue.back());
		queue.pop_front();
		value = queue.front();
	}
}



XMLNode DelayProperty::storeData() const
{
	XMLNode node = XMLNode::createXMLTopNode(XMLDataName().c_str());
	node.addAttribute("symbol-ref",symbolic_name.c_str());
	node.addAttribute("value",to_cstr(queue));
	return node;
}

void DelayProperty::restoreData(XMLNode parent_node)
{
	XMLNode node = parent_node.getChildNodeWithAttribute(XMLDataName().c_str(),"symbol-ref",symbolic_name.c_str());
	if (!node.isEmpty()) {
		if ( getXMLAttribute(node,"value",queue,false)) {
			cout << symbolic_name << "=" << queue << "; ";
		} else {
			cerr << XMLName() << "::restoreData: Cannot restore " << XMLDataName() << "of referenced symbol \"" << symbolic_name << endl;
		}
	}
}





// template <> const string Property<double_queue>::property_xml_name() { return "PropertyQueue";};
// template <> const string Property<double_queue>::global_xml_name() { return "GlobalQueue";};
// template <> const string Property<double_queue>::global_constant_xml_name() { return "ConstantQueue";};
//
