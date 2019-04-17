#include "core/interfaces.h"
#include "core/cell.h"


 /* 
 * This documentation contains the documentation of the plugin's XML language specification
 * It is parsed via doxygen, compiled for the QtHelp system and then compiled into the GUI.
 * 
 * (A) Create a documentation group based on the XML tag name. Good praxis is to define the documentation group as
 *  \defgroup ML_[PluginName] [PluginName] 
 *  The first name is the tag used to refer to the Plugin within the documentation, and the 
 *  second is required to be the XMLTag/XMLName of the Plugin
 *  
 * 
 * (B) Refer to the parental XML elements that shall allow the specification of your plugin (See XSD Group specification)
 *  *    XSD group             Docu
 *  *  GlobalPlugins   ->  \ingroup ML_Global
 *  *  CellTypePlugin  ->  \ingroup ML_CellType
 *  *  AnalysisPlugin  ->  \ingroup ML_Analysis
 *  *  SystemPlugins   ->  \ingroup ML_System
 *  *  ...
 * 
 * (C) Document the usage of your plugin.
*/
 
/** \defgroup ML_BBox BoundingBox
\ingroup Symbol
\ingroup ML_CellTyope
\brief Custom symbol to access a cell's bounding box coordinates

  * \b min-symbol  -  symbol name for the lower edge of the bounding box
  * \b max-symbol  -  symbol name for the lower edge of the bounding box
*/


class BBoxSymbol : public Plugin {
	
public:
	DECLARE_PLUGIN("BBox");
	BBoxSymbol(): Plugin() { 
		// Interface with the XML definition
		min_symbol_name.setXMLPath("min-symbol");
		max_symbol_name.setXMLPath("max-symbol");
		registerPluginParameter(min_symbol_name); 
		registerPluginParameter(max_symbol_name); 
	}

	void loadFromXML(const XMLNode node, Scope * scope) override {
		Plugin::loadFromXML(node,scope);
		
		min_symbol = make_shared<Symbol>(min_symbol_name(), "lower edge of the cells boundary box", Symbol::Mode::MIN );
		scope->registerSymbol(min_symbol);
		max_symbol = make_shared<Symbol>(max_symbol_name(), "upper edge of the cells boundary box", Symbol::Mode::MAX );
		scope->registerSymbol(max_symbol);
 // <- added description
	}
	
// 	void init(const Scope * scope) override {};
	// Create a read-only symbol accessor
	class Symbol : public SymbolAccessorBase<VDOUBLE> { 
	public:
		enum class Mode {MIN, MAX};
		Symbol(string name, string descr, Mode mode): SymbolAccessorBase<VDOUBLE>(name), descr(descr), mode(mode) {
			// Carefully set flags in order to describe the symbols properties (also see SymbolBase::Flags)
			this->flags().granularity = Granularity::Cell;
		};
		// Provide a description for output generation
		const string&  description() const override { return descr; }

		// For logging info creation
		std::string linkType() const override { return "BBoxLink"; } 

		// The standard accessor for the symbol
		TypeInfo<VDOUBLE>::SReturn get(const SymbolFocus & f) const override {  // << -- The actual implementation! Note the return type!
			//return f.cell().nNodes(); 
			
			if (mode == Mode::MIN ) {
				const auto& nodes = f.cell().getNodes();
				if (nodes.empty())
					return VDOUBLE(0,0,0);
				
				// initialize min and max of bounding box
				VDOUBLE bbmin = *nodes.begin();
				
				// iterate through cell nodes to find min/max along each x,y,z axis
				for(auto n : nodes){
					if(n.x < bbmin.x) bbmin.x = n.x;
					if(n.y < bbmin.y) bbmin.y = n.y;
					if(n.z < bbmin.z) bbmin.z = n.z;
				}
				return bbmin;
			}
			else {
				const auto& nodes = f.cell().getNodes();
				if (nodes.empty())
					return VDOUBLE(0,0,0);
				
				// initialize min and max of bounding box
				VDOUBLE bbmax = *nodes.begin();
				
				// iterate through cell nodes to find min/max along each x,y,z axis
				for(auto n : nodes){
					if(n.x > bbmax.x) bbmax.x = n.x;
					if(n.y > bbmax.y) bbmax.y = n.y;
					if(n.z > bbmax.z) bbmax.z = n.z;
				}
				return bbmax;
			}
		}
		
		// During initialisation phase, this accessor is used. You have to make sure that the symbol is initialized, because the Plugins init method may not have been called yet.
		TypeInfo<VDOUBLE>::SReturn safe_get(const SymbolFocus & f) const override {
			// We don't need no initialisation for the BBoxPlugin
			return get(f);
		};
		
		
		// For writable symbols, aka derived from SymbolRWAccessorBase<T>, the following methods also have to be implemented
		
		// void set(const SymbolFocus & f, typename TypeInfo<double>::Parameter value) const override { };
		// void setBuffer(const SymbolFocus & f, TypeInfo<double>::Parameter value) const override { }
		// void applyBuffer() const override { };
		// void applyBuffer(const SymbolFocus & f) const override { }
		
	private: 
		string descr;
		Mode mode;
	};
	
private:
	PluginParameter2<string, XMLValueReader, RequiredPolicy> min_symbol_name;
	PluginParameter2<string, XMLValueReader, RequiredPolicy> max_symbol_name;
	shared_ptr<Symbol> min_symbol;
	shared_ptr<Symbol> max_symbol;
};
