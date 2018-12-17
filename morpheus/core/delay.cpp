#include "delay.h"

ostream& operator<<(ostream& out, const DelayData& d ) {
	out <<"(" << d.time << "," <<  d.value << ")";
	return out;
}

istream& operator>>(istream& in, DelayData& d) {
	char c;
	in >> c >> d.time >> c >> d.value >> c;
	return in;
}


DelayPropertyPlugin::DelayPropertyPlugin(Mode mode): Container<double>(mode), ContinuousProcessPlugin(ContinuousProcessPlugin::DELAY, XMLSpec::XML_NONE), tsl_initialized(false) {

};

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
			auto property = make_shared<DelayProperty>(this, DelayBuffer(10));
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
	
	vector<EvaluatorVariable> table  {{SymbolBase::Time_symbol, EvaluatorVariable::DOUBLE}};
	value.setLocalsTable(table);

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
	
	if (mode == Mode::Variable) {
		static_pointer_cast<DelayVariableSymbol>(_accessor)->init();
	}
	else if (mode == Mode::CellProperty) {
		static_pointer_cast<DelayPropertySymbol>(_accessor)->init();
	}
};

double DelayPropertyPlugin::getInitValue(const SymbolFocus f, double time) {
	if (! this->initialized) init(local_scope);
	value.setLocals(&time);
	return value.safe_get(f);
}

template<>
void DelayProperty::init(const SymbolFocus& f) {
	auto time = SIM::getTime();
	auto real_parent = static_cast<DelayPropertyPlugin*>(parent);
	auto delay = real_parent->getDelay();
	
	if (this->value.capacity()< 11) this->value.set_capacity(11);
	this->value.clear();
	for (int i=0; i<11; i++) {
		double t = time - (1-i/10.0) * delay;
		this->value.push_back( { t, real_parent->getInitValue(f, t)} );
	}
	initialized = true;
	cout << "Initialized Delay " << to_str(this->value) << endl;
};


void DelayPropertyPlugin::setTimeStep(double t)
{
	ContinuousProcessPlugin::setTimeStep(t);
// 	int queue_length;
// 	if (t==0) {
// 		queue_length = 2;
// 	}
// 	else {
// 		assert(t<=delay);
// 		queue_length = floor(delay/t);
// 		t = delay / queue_length;
// 		queue_length += 1; // need one more storage locations than intervals;
// 	}
// 	//TODO This resizeing of the downstream containers should be done by the Symbols !!! However, the value sits here in the Plugin.
// 	if (mode==Mode::Variable) {
// 		static_pointer_cast<DelayVariableSymbol>(_accessor)->property.value.resize(queue_length,value.safe_get(SymbolFocus::global));
// 	}
// 	else if (mode == Mode::CellProperty) {
// 		const CellType*  celltype = scope()->getCellType();
// 		
// 		static_pointer_cast<DelayProperty>(celltype->default_properties[property_id])->value.resize(queue_length,value.safe_get(SymbolFocus::global));
// 		// Assume there are no cells yet ...
// 		for (auto cell_id : celltype->getCellIDs()) {
// 			static_pointer_cast<DelayProperty>(CPM::getCell(cell_id).properties[property_id])->value.resize(queue_length,value.safe_get(SymbolFocus(cell_id)));
// 		}
// 	}
// 	cout << "Queue for DelayProperty " << getSymbol() << " resized to length " << queue_length << " with step size " << timeStep() << endl;
}

// void DelayPropertyPlugin::executeTimeStep()
// {
// 	if (mode == Mode::CellProperty) {
// 		const CellType*  celltype = scope()->getCellType();
// 		//  Execute time step for all cloned containers attached to cells
// 		for (auto cell_id : celltype->getCellIDs()) {
// 			auto property = static_pointer_cast<DelayProperty>(CPM::getCell(cell_id).properties[property_id]);
// 			property->value.push_back(property->value.back());
// 			property->value.pop_front();
// 		}
// 	}
// 	else {
// 		auto property = &(static_pointer_cast<DelayVariableSymbol>(_accessor)->property);
// 		property->value.push_back(property->value.back());
// 		property->value.pop_front();
// 	}
// }

