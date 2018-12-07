#include "property.h"
// #include "expression_evaluator.h"

template <> string Container<double>::ConstantXMLName()  { return "Constant";};
template <> string Container<VDOUBLE>::ConstantXMLName() { return "ConstantVector";};
template <> string Container<double>::VariableXMLName() { return "Variable";};
template <> string Container<VDOUBLE>::VariableXMLName() { return "VariableVector";};
template <> string Container<double>::CellPropertyXMLName() { return "Property";};
template <> string Container<VDOUBLE>::CellPropertyXMLName() { return "PropertyVector";};

template <> string Container<vector<double> >::ConstantXMLName() { return "ConstantArray";};
template <> string Container<vector<double> >::VariableXMLName() { return "VariableArray";};
template <> string Container<vector<double> >::CellPropertyXMLName() { return "PropertyArray";};

template <> 
bool Container<double>::type_registration = PluginFactory::RegisterCreatorFunction( Container<double>::ConstantXMLName(),Container<double>::createConstantInstance) 
								   && PluginFactory::RegisterCreatorFunction( Container<double>::VariableXMLName(), Container<double>::createVariableInstance)
								   && PluginFactory::RegisterCreatorFunction( Container<double>::CellPropertyXMLName(), Container<double>::createCellPropertyInstance);

template <> 
bool Container<VDOUBLE>::type_registration = PluginFactory::RegisterCreatorFunction( Container<VDOUBLE>::ConstantXMLName(),Container<VDOUBLE>::createConstantInstance) 
								   && PluginFactory::RegisterCreatorFunction( Container<VDOUBLE>::VariableXMLName(), Container<VDOUBLE>::createVariableInstance)
								   && PluginFactory::RegisterCreatorFunction( Container<VDOUBLE>::CellPropertyXMLName(), Container<VDOUBLE>::createCellPropertyInstance);

DelayPropertyPlugin::DelayPropertyPlugin(Mode mode): Container<double>(mode), ContinuousProcessPlugin(ContinuousProcessPlugin::DELAY, XMLSpec::XML_NONE), tsl_initialized(false) {};

Plugin* DelayPropertyPlugin::createVariableInstance(){ return new DelayPropertyPlugin(Mode::Variable) ; };

Plugin* DelayPropertyPlugin::createCellPropertyInstance(){ return new DelayPropertyPlugin(Mode::CellProperty) ; };

bool DelayPropertyPlugin::type_registration =
	PluginFactory::RegisterCreatorFunction( DelayPropertyPlugin::VariableXMLName(), DelayPropertyPlugin::createVariableInstance)
	&& PluginFactory::RegisterCreatorFunction( DelayPropertyPlugin::CellPropertyXMLName(), DelayPropertyPlugin::createCellPropertyInstance);

void DelayPropertyPlugin::loadFromXML(XMLNode node, Scope* scope)
{
	ContinuousProcessPlugin::loadFromXML(node, scope);
	
// 	Container< double >::loadFromXML(node, scope);
	Plugin::loadFromXML(node, scope);
	
	switch (mode) {
		case Mode::Variable : 
			_accessor = make_shared<DelayVariableSymbol>(this);
			break;
		case Mode::CellProperty : {
			auto ct = scope->getCellType();
			if (! ct)
				throw MorpheusException(CellPropertyXMLName() + " requires to be defined within acelltype scope ", node);
			auto property = make_shared<DelayProperty>(this,deque<double>(2,0));
			property_id = ct->addProperty(property);
			_accessor = make_shared<DelayPropertySymbol>(this,ct,property_id);
			break;
		}
		case Mode::Constant :
			throw string("No constant DelayProperty.");
	}
	
	scope->registerSymbol(_accessor);
	registerInputSymbol(_accessor);
	registerOutputSymbol(_accessor);

	if (!getXMLAttribute(node,"delay",delay) ) {
		delay =0;
	}
	
	
	if (!type_registration)
		cout << "Don't ever remove me! " << " I take care to register this Plugin !!" << endl;
}

void DelayPropertyPlugin::init(const Scope* scope) {
	
	if ( ! tsl_initialized ) {
		Plugin::init(scope);
		this->initialized = true;
		ContinuousProcessPlugin::init(scope);
		tsl_initialized = true;
	}
	setTimeStep(delay);
	
	DelayProperty* property;
	if (mode == Mode::Variable) {
		static_pointer_cast<DelayVariableSymbol>(_accessor)->init();
	}
	else if (mode == Mode::CellProperty) {
		static_pointer_cast<DelayPropertySymbol>(_accessor)->init();
	}
};

template <>
void DelayProperty::init(const SymbolFocus& f) {
	std::fill(this->value.begin(),this->value.end(),parent->getInitValue(f));
	initialized = true;
};


void DelayPropertyPlugin::setTimeStep(double t)
{
	ContinuousProcessPlugin::setTimeStep(t);
	int queue_length;
	if (t==0) {
		queue_length = 2;
	}
	else {
		assert(t<=delay);
		queue_length = floor(delay/t);
		t = delay / queue_length;
		queue_length += 1; // need one more storage locations than intervals;
	}
	//TODO This resizeing of the downstream containers should be done by the Symbols !!! However, the value sits here in the Plugin.
	if (mode==Mode::Variable) {
		static_pointer_cast<DelayVariableSymbol>(_accessor)->property.value.resize(queue_length,value.safe_get(SymbolFocus::global));
	}
	else if (mode == Mode::CellProperty) {
		const CellType*  celltype = scope()->getCellType();
		
		static_pointer_cast<DelayProperty>(celltype->default_properties[property_id])->value.resize(queue_length,value.safe_get(SymbolFocus::global));
		// Assume there are no cells yet ...
		for (auto cell_id : celltype->getCellIDs()) {
			static_pointer_cast<DelayProperty>(CPM::getCell(cell_id).properties[property_id])->value.resize(queue_length,value.safe_get(SymbolFocus(cell_id)));
		}
	}
	cout << "Queue for DelayProperty " << getSymbol() << " resized to length " << queue_length << " with step size " << timeStep() << endl;
}

void DelayPropertyPlugin::executeTimeStep()
{
	if (mode == Mode::CellProperty) {
		const CellType*  celltype = scope()->getCellType();
		//  Execute time step for all cloned containers attached to cells
		for (auto cell_id : celltype->getCellIDs()) {
			auto property = static_pointer_cast<DelayProperty>(CPM::getCell(cell_id).properties[property_id]);
			property->value.push_back(property->value.back());
			property->value.pop_front();
		}
	}
	else {
		auto property = &(static_pointer_cast<DelayVariableSymbol>(_accessor)->property);
		property->value.push_back(property->value.back());
		property->value.pop_front();
	}
}
