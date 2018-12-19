#ifndef DELAY_H
#define DELAY_H

#include "property.h"

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
	double getInitValue(const SymbolFocus f, double time);
	
	void executeTimeStep() override {};
	void prepareTimeStep() override {};
	
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
		if (history[0].time > time-delay) {
			if (time-delay<SIM::getStartTime()) 
				history.push_front({time-delay,parent->getInitValue(f,time-delay)});
			else 
				history.push_front({time-delay, history[0].value});
		}
		cout << "Delay " << this->name() << "={"; 
		for (const auto& h: history) {
			cout << h.time << "->" << h.value << ", ";
		}
		cout << "}" << endl;
// 		if (history.size() == 1) return history[0].value;
		const auto& h0 = history[0]; const auto& h1 = history[1];
		auto fraction = ((time - delay) - h0.time) / (h1.time - h0.time + 1e-25);
		return h0.value * (1-fraction) + h1.value * fraction;
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
		while (history.size()>1 && history[1].time - 1e-25 <= time-parent->getDelay()) history.pop_front();
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
		
		if (history[0].time > time-delay) {
			if (time-delay < SIM::getStartTime())
				history.push_front({time-delay,parent->getInitValue(f,time-delay)});
			else
				history.push_front({time-delay,history[0].value});
		}
		cout << "Delay " << this->name() << "={"; 
		for (const auto& h: history) {
			cout << h.time << "->" << h.value << ", ";
		}
		cout << "}" << endl;
		
		if (history.size() == 1) return history[0].value;
		auto fraction = ((time - delay) - history[0].time) / (history[1].time - history[0].time + 1e-25);
		return history[0].value * (1-fraction) + history[1].value * fraction;
		
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
		while (property.value.size()>1 && property.value[1].time - 1e-25<=time-parent->getDelay()) property.value.pop_front();
	}
	friend class DelayPropertyPlugin;
};

#endif // DELAY_H
