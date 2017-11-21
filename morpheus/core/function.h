//////
//
// This file is part of the modelling and simulation framework 'Morpheus',
// and is made available under the terms of the BSD 3-clause license (see LICENSE
// file that comes with the distribution or https://opensource.org/licenses/BSD-3-Clause).
//
// Authors:  Joern Starruss and Walter de Back
// Copyright 2009-2016, Technische Universität Dresden, Germany
//
//////

/**
\defgroup Function
\ingroup ML_Global
\ingroup ML_CellType
\ingroup ML_System
\ingroup ML_Event
\ingroup ML_Contact
\ingroup ML_Analysis
\ingroup MathExpressions
\ingroup Symbols

Symbol that defines a relation between \ref Symbols. 

Functions are not explicitly scheduled. Instead they are evaluated 'on-the-fly' whenever their output symbols are used.


For vector data, use \ref ML_VectorFunction.

To assign to a variable or property, use \ref ML_Equation.

\section Example
Assume 'a' is a variable or property.
\verbatim
<Function symbol="a">
	(u*v)/(1+v)
</Function>  
\endverbatim

**/

/**
\defgroup ML_VectorFunction VectorFunction
\ingroup ML_Global
\ingroup ML_CellType
\ingroup ML_System
\ingroup ML_Event
\ingroup ML_Analysis
\ingroup MathExpressions
\ingroup Symbols

Symbol that defines a relation between vector \ref Symbols
**/

#ifndef Function_H
#define Function_H
#include "core/interfaces.h"
#include "expression_evaluator.h"


class FunctionPlugin : public Plugin {
	public:
		DECLARE_PLUGIN("Function");
		
		XMLNode saveToXML() const override;
		void loadFromXML(const XMLNode, Scope* scope) override;
		void init () { init(local_scope); }; // used for on-demand init by the accessor
		void init (const Scope* scope) override;

		string getExpr() const {return raw_expression();};
		const string& getSymbol()  const { return symbol(); }
// 		Granularity getGranularity() const { if (evaluator) return evaluator->getGranularity(); else return Granularity::Global; };
	
		class Accessor: public SymbolAccessorBase<double> {
			public:
				Accessor(FunctionPlugin* parent) : SymbolAccessorBase<double>(parent->getSymbol()), parent(parent) {};
				double safe_get(const SymbolFocus& focus) const override { if (!evaluator) parent-> init(); return evaluator->get(focus); }
				double get(const SymbolFocus& focus) const override { return evaluator->get(focus); }
				std::set<SymbolDependency> dependencies() const override { if (!evaluator) parent-> init(); return evaluator->getDependSymbols();};
				const std::string & description() const override { return parent->getDescription(); }
				std::string linkType() const override { return "FunctionLink"; }
			private:
				void setEvaluator(shared_ptr<ThreadedExpressionEvaluator<double> > e) { evaluator = e; flags().granularity = evaluator->getGranularity(); };
				shared_ptr<ThreadedExpressionEvaluator<double> > evaluator;
				FunctionPlugin* parent;
				friend class FunctionPlugin;
		};
	private:
		shared_ptr<Accessor> accessor;
		shared_ptr<ThreadedExpressionEvaluator<double> > evaluator;
		PluginParameter2<string, XMLValueReader, RequiredPolicy> raw_expression;
		PluginParameter2<string, XMLValueReader, RequiredPolicy> symbol;
};

class VectorFunction : public Plugin
{
	public:
		DECLARE_PLUGIN("VectorFunction");

		void loadFromXML(const XMLNode, Scope* scope) override;
		void init() { init(local_scope); }
		void init (const Scope* scope) override;
	
		string getExpr() const { return raw_expression(); };
		bool isSpherical() const { return is_spherical(); }
		const string& getSymbol()  const { return symbol(); }
// 		Granularity getGranularity() const { if (evaluator) return evaluator->getGranularity(); else return Granularity::Global; };

		class Accessor: public SymbolAccessorBase<VDOUBLE> {
			public:
				Accessor(VectorFunction* parent) : SymbolAccessorBase<VDOUBLE>(parent->getSymbol()), parent(parent) {};
				TypeInfo<VDOUBLE>::SReturn safe_get(const SymbolFocus& focus) const override { if (!evaluator) parent-> init(); return evaluator->get(focus); }
				TypeInfo<VDOUBLE>::SReturn get(const SymbolFocus& focus) const override { return is_spherical ? VDOUBLE::from_radial(evaluator->get(focus)) : evaluator->get(focus); }
				std::set<SymbolDependency> dependencies() const override { if (!evaluator) parent-> init(); return evaluator->getDependSymbols();};
				const std::string & description() const override { return parent->getDescription(); }
				std::string linkType() const override { return "VectorFunctionLink"; }
			private:
				void setEvaluator(shared_ptr<ThreadedExpressionEvaluator<VDOUBLE> > e) { evaluator = e; flags().granularity = evaluator->getGranularity(); };
				shared_ptr<ThreadedExpressionEvaluator<VDOUBLE> > evaluator;
				bool is_spherical;
				VectorFunction* parent;
				friend class VectorFunction;
		};
		
	private:
		shared_ptr<ThreadedExpressionEvaluator<VDOUBLE> > evaluator;
		PluginParameter2<bool, XMLValueReader, DefaultValPolicy> is_spherical;
		PluginParameter2<string, XMLValueReader, RequiredPolicy> raw_expression;
		PluginParameter2<string, XMLValueReader, RequiredPolicy> symbol;
		string description;
		shared_ptr<Accessor> accessor;
};

#endif
