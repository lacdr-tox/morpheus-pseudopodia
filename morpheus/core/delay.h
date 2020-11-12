#ifndef DELAY_H
#define DELAY_H

#include "property.h"
#include <boost/circular_buffer.hpp>
/**
\defgroup ML_DelayVariable DelayVariable
\ingroup ML_Global
\ingroup Symbols

Symbol with a scalar value and a \b delay time before assigned values become current. The initial value and history is given by a \ref MathExpressions.
**/

/**
\defgroup ML_DelayProperty DelayProperty
\ingroup ML_CellType
\ingroup Symbols


Symbol with a cell-bound scalar value and a \b delay time before assigned values become current. The initial value and history is given by a \ref MathExpressions
**/



struct DelayData {
	double time;
	double value;
};

ostream& operator<<(ostream& out, const DelayData& d );

istream& operator>>(istream& in, DelayData& d);

using DelayBuffer= boost::circular_buffer<DelayData>;

typedef Property<double, DelayBuffer> DelayProperty;

template <>
void DelayProperty::init(const SymbolFocus& f) ;

class DelayPropertyPlugin : public Container<double>, public ContinuousProcessPlugin
{
protected:
	DelayPropertyPlugin(Mode mode);
	
	// manual replacement for the DECLARE_PLUGIN macro
	static bool type_registration;
	static const string CellPropertyXMLName() { return "DelayProperty"; };
	static const string VariableXMLName() { return "DelayVariable"; };
	static Plugin* createVariableInstance();
	static Plugin* createCellPropertyInstance();

	PluginParameter<double, XMLEvaluator, RequiredPolicy> delay;
	bool tsl_initialized;

public:
	string XMLName() const override { if (mode==Mode::Variable) return VariableXMLName(); else return  CellPropertyXMLName(); }

	void setTimeStep(double t) override;
	double getDelay() const { return delay(SymbolFocus::global);}
	double getInitValue(const SymbolFocus& f, double time);
	
	void prepareTimeStep(double step_size) override {};
	void executeTimeStep() override {};
	
	void loadFromXML(XMLNode node, Scope* scope) override;
	void init(const Scope* scope) override;
	void assert_initialized();
};

class DelayPropertySymbol : public SymbolRWAccessorBase<double> {
public:
	DelayPropertySymbol(DelayPropertyPlugin* parent, const CellType* ct, uint pid) : 
		SymbolRWAccessorBase<double>(parent->getSymbol()), parent(parent), celltype(ct), property_id(pid)
	{ 
		this->flags().delayed = true;
		this->flags().granularity = Granularity::Cell;
	};
	std::string linkType() const override { return "DelayPropertyLink"; }
	const string& description() const override { if (parent) return parent->getName();  return this->name();}
	double get(const SymbolFocus& f) const override {
		return get(f, SIM::getTime());
	}
	double get(const SymbolFocus& f, double time) const {
		auto& history = getCellProperty(f)->value;
		auto delay = parent->getDelay();
		clearHistory(history, time);
		if (time-delay<=SIM::getStartTime()) {
// 			cout << "Delay ini " << this->name() << " -> {" << time-delay << "," <<  parent->getInitValue(f,time-delay) <<"}";
			return parent->getInitValue(f,time-delay);
		}
		else if (history[0].time > time-delay) {
			history.push_front({time-delay, history[0].value});
		}
		
		double r;
		if (history.size() == 1) {
			r=history[0].value;
		}
		else {
			int i=0;
			while ((history.size()>i+1) and history[i+1].time<time-delay) i++;
			auto fraction = ((time - delay) - history[i].time) / (history[i+1].time - history[i].time + 1e-25);
			r =  history[i].value * (1-fraction) + history[i+1].value * fraction;
		}
// 		cout << "Delay " << this->name() << " -> {" << time-delay << "," <<  r <<"}" << endl;
		return r;
	}
	double safe_get(const SymbolFocus& f) const override {
		auto p=getCellProperty(f); 
		if (!p->initialized)
			p->init(f);
		return get(f);
	}
	void init() { 
		DelayProperty* p = static_cast<DelayProperty*>(celltype->default_properties[property_id].get());
		if (!p->initialized) 
			try {
				p->init(SymbolFocus::global);
			} catch (...) { cout << "Warning: Could not initialize default property " << this->name() << " of CellType " << celltype->getName() << "." << endl;}
	}
	void set(const SymbolFocus& f, double value) const override {
		set(f, SIM::getTime(), value);
	}
	void set(const SymbolFocus& f, double time, double value) const { 
		auto& history = getCellProperty(f)->value;
		clearHistory(history,time);
		if (history.full()) history.set_capacity((history.size()*4)/3);
		history.push_back({time, value});
	};
	void setBuffer(const SymbolFocus& f, double value) const override { set(f,value); };
	void applyBuffer() const override {};
	void applyBuffer(const SymbolFocus&) const override {};
private:
	DelayProperty* getCellProperty(const SymbolFocus& f) const { 
		return static_cast<DelayProperty*>(f.cell().properties[property_id].get()); 
	}
	void clearHistory(DelayBuffer& history, double time) const {
		// auto cleanup
		while (history.size()>1 && (history[1].time + max(1.0,0.1*time-parent->getDelay()))<=time-parent->getDelay()) history.pop_front();
	}
	DelayPropertyPlugin* parent;
	const CellType* celltype;
	uint property_id;
	friend class DelayPropertyPlugin;
};

class DelayVariableSymbol : public SymbolRWAccessorBase<double>  {
public:
	DelayVariableSymbol(DelayPropertyPlugin* parent) : SymbolRWAccessorBase<double>(parent->getSymbol()), parent(parent), property(parent, DelayBuffer(10)) {
		this->flags().delayed = true;
	};
	std::string linkType() const override { return "DelayVariableLink"; }
	const string& description() const override { if (parent) return parent->getName();  return this->name();}
	double get(const SymbolFocus& f) const override {
		return get(f, SIM::getTime());
	}
	double get(const SymbolFocus& f, double time) const {
		auto delay = parent->getDelay();
		clearHistory(time);
		auto& history = property.value;
		
		if (time-delay<=SIM::getStartTime()) {
// 			cout << "Delay ini " << this->name() << " -> {" << time-delay << "," <<  parent->getInitValue(f,time-delay) <<"}" << endl;
			return parent->getInitValue(f,time-delay);
		}
		else if (history[0].time > time-delay) {
			history.push_front({time-delay, history[0].value});
		}
		double r;
		if (history.size() == 1) {
			r=history[0].value;
		}
		else {
			int i=0;
			while ((history.size()>i+1) and history[i+1].time<time-delay) i++;
			auto fraction = ((time - delay) - history[i].time) / (history[i+1].time - history[i].time + 1e-25);
			r =  history[i].value * (1-fraction) + history[i+1].value * fraction;
		}
// 		cout << "Delay " << this->name() << " -> {" << time-delay << "," <<  r <<"}" << endl;
		return r;
		
	}
	double safe_get(const SymbolFocus& f) const override {
		if (!property.initialized) {
			property.init(SymbolFocus::global);
		}
		return get(f);
	}
	void init() { if (!property.initialized) property.init(SymbolFocus::global); }
	void set(const SymbolFocus& f, double value) const override {
		set(f, SIM::getTime(), value);
	}
	void set(const SymbolFocus&, double time, double value) const {
		auto& history = property.value;
		clearHistory(time-parent->getDelay());
		if (history.full()) history.set_capacity((history.size() * 4)/3);
		history.push_back({time,value});
	};
	void setBuffer(const SymbolFocus& f, double value) const override { set(f,value); };
	void applyBuffer() const override {};
	void applyBuffer(const SymbolFocus&) const override {};
private:
	DelayPropertyPlugin* parent;
	mutable DelayProperty property;
	void clearHistory(double time) const {
		// auto cleanup
		while (property.value.size()>1 && (property.value[1].time + max(1.0,0.1*time-parent->getDelay()))<=time-parent->getDelay()) property.value.pop_front();
	}
	friend class DelayPropertyPlugin;
};

#endif // DELAY_H
