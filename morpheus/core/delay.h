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

	double delay;
	bool tsl_initialized;

public:
	string XMLName() const override { if (mode==Mode::Variable) return VariableXMLName(); else return  CellPropertyXMLName(); }

	void setTimeStep(double t) override;
	double getDelay() const { return delay;}
	double getInitValue(const SymbolFocus f, double time);
	
	void executeTimeStep() override {};
	void prepareTimeStep() override {};
	
	void loadFromXML(XMLNode node, Scope* scope) override;
	void init(const Scope* scope) override;
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
		auto& history = getCellProperty(f)->value;
		auto time = SIM::getTime();
		clearHistory(history, time);
		if (history.size() == 1) return history[0].value;
		const auto& h0 = history[0]; const auto& h1 = history[1];
		auto fraction = ((time - parent->getDelay()) - h0.time) / (h1.time - h0.time);
		return h0.value * fraction + h1.value * (1-fraction);
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
		DelayData d = { SIM::getTime(), value};
		auto& history = getCellProperty(f)->value;
		clearHistory(history,d.time);
		if (history.full()) history.set_capacity((history.size()*4)/3);
		history.push_back(d);
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
	double get(const SymbolFocus&) const override {
		auto time = SIM::getTime();
		clearHistory(time);
		auto& history = property.value;
		if (history.size() == 1) return history[0].value;
		auto fraction = ((time - parent->getDelay()) - history[0].time) / (history[1].time - history[0].time);
		return history[0].value * fraction + history[1].value * (1-fraction);
		
	}
	double safe_get(const SymbolFocus& f) const override {
		if (!property.initialized) {
			property.init(SymbolFocus::global);
		}
		return get(f);
	}
	void init() { if (!property.initialized) property.init(SymbolFocus::global); }
	void set(const SymbolFocus&, double value) const override {
		auto& history = property.value;
		if (history.full()) history.set_capacity((history.size() * 4)/3);
		history.push_back({SIM::getTime(),value});
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
