//////
//
// This file is part of the modelling and simulation framework 'Morpheus',
// and is made available under the terms of the BSD 3-clause license (see LICENSE
// file that comes with the distribution or https://opensource.org/licenses/BSD-3-Clause).
//
// Authors:  Joern Starruss and Walter de Back
// Copyright 2009-2018, Technische Universität Dresden, Germany
//
//////

/**
\defgroup ML_Function Function
\ingroup ML_Global
\ingroup ML_CellType
\ingroup ML_System
\ingroup ML_Event
\ingroup ML_Contact
\ingroup ML_Analysis
\ingroup MathExpressions
\ingroup Symbols

\brief Parametric Function declaration.

Functions define an expression that relate Parameters, \ref Symbols from the \ref Scope of the Function definition and other *Function*s to a scalar result. A Function captures the scope of it's definition, thus applying a Function in a sub-scope will not make the sub-scope's symbol definitions available.

A kind of exception is spatial scoping, where sub-scope symbols of spatial regions (celltype symbols) are promoted top the parental scopes. 


Function definitions are available within the local \ref Scope and all sub-scopes therein.

For convenience, a parameter-free Function definition is also available as a plain Symbol, thus you may call it without the paretheses. 

Functions are not explicitly scheduled. Instead they are evaluated 'on-the-fly' whenever their output requested.


For vector data, use \ref ML_VectorFunction.

To assign to a variable or property, use \ref ML_Equation.

\section Example

A non-parametric function avalable as **a** and **a()** in mathematical expressions
\verbatim
<Field symbol="u" />
<Variable symbol="v" value="2.4" />
<Function symbol="a">
	(u*v)/(1+v)
</Function>  
\endverbatim


A parametric function available as **a(k1,k2)** in mathematical expressions
\verbatim

<Function symbol="a">
	<Parameter "k1" />
	<Parameter "k2" />
	<Expression> 
		1/(1+k1)(1+k2^2)
	</Expression>
</Function>  


\endverbatim

**/
/*
 * Not yet supported
Scalar or dot product of two vector parameters
<Function symbol="dot">
	<VectorParameter "a" />
	<VectorParameter "b" />
	<Expression> 
		a.x*b.x+a.y*b.y+a.z*b.z
	</Expression>
</Function>  
*/
/**
\defgroup ML_VectorFunction VectorFunction
\ingroup ML_Global
\ingroup ML_CellType
\ingroup ML_System
\ingroup ML_Event
\ingroup ML_Analysis
\ingroup MathExpressions
\ingroup Symbols

Symbol that defines a relation between vector \ref Symbols from the \ref Scope of the VectorFunction definition and other *Function*s to a scalar result.
**/

#ifndef Function_H
#define Function_H
#include "core/interfaces.h"
#include "expression_evaluator.h"


// template <class source, class target, class trans >
// void transform_all(const source& s, target& t, trans&& tr) {
// 	std::transform(s.begin(), s.end(), back_inserter(t), tr);
// }
class FunctionPlugin : public Plugin {
	public:
		DECLARE_PLUGIN("Function");
		~FunctionPlugin() { if (accessor && accessor->scope()) const_cast<Scope*>(accessor->scope())->removeSymbol(accessor); }
		
		XMLNode saveToXML() const override;
		void loadFromXML(const XMLNode, Scope* scope) override;
		void init () { init(local_scope); }; // used for on-demand init by the accessor
		void init (const Scope* scope) override;

		string getExpr() const {return raw_expression();};
		vector<string> getParams() const { 
			vector<string> names;
			std::transform (parameters.begin(), parameters.end(),back_inserter(names), [] (FunParameter const & param) { return param.symbol();});
			return names;
		}
		const string& getSymbol()  const { return symbol(); }
// 		Granularity getGranularity() const { if (evaluator) return evaluator->getGranularity(); else return Granularity::Global; };
	
		class Symbol: public SymbolAccessorBase<double>, public FunctionAccessor<double> {
			public:
				Symbol(FunctionPlugin* parent) : SymbolAccessorBase<double>(parent->getSymbol()), parent(parent) { flags().function = true; };
				// Standard symbol interface, used for parameter-free functions, which is merely an alias
				double safe_get(const SymbolFocus& focus) const override { if (!evaluator) parent-> init(); return evaluator->get(focus); }
				double get(const SymbolFocus& focus) const override { return evaluator->get(focus); }
				
				// Interface for the parametric function call
				int parameterCount() const override { return evaluator->getLocalsCount(); }
				mu::fun_class_generic* getCallBack() const override {
					if (!callback) {
						if (!evaluator)
							parent->init();
						callback = make_unique<CallBack>(evaluator);
					}
					return callback.get();
				}
				double get(double parameters[], const SymbolFocus& focus) const override { evaluator->setLocals(parameters); return evaluator->get(focus); };
				double safe_get(double parameters[], const SymbolFocus& focus) const override { if (!evaluator) parent-> init(); return get(parameters, focus);};
				
				std::set<SymbolDependency> dependencies() const override { if (!evaluator) parent-> init(); return evaluator->getDependSymbols();};
				const std::string & description() const override { return parent->getDescription(); }
				std::string linkType() const override { return "FunctionLink"; }
				
			private:
				/// the callback class to be passed to muparser function definition
				class CallBack : public mu::fun_class_generic {
					public:
						CallBack(shared_ptr<ThreadedExpressionEvaluator<double> > evaluator) : evaluator(evaluator) {};
						double operator()(const double* args, void * data) const override {
							evaluator->setLocals(args);
							return evaluator->get(*reinterpret_cast<const SymbolFocus*>(data));
						};
						int argc() const override {
							return evaluator->getLocalsCount();
						};
						
					private:
						shared_ptr<ThreadedExpressionEvaluator<double> > evaluator;
				};
				void setEvaluator(shared_ptr<ThreadedExpressionEvaluator<double> > e) {
					evaluator = e;
					flags().granularity = evaluator->getGranularity();
				};
				shared_ptr<ThreadedExpressionEvaluator<double> > evaluator;
				mutable unique_ptr<CallBack> callback;
				FunctionPlugin* parent;
				friend class FunctionPlugin;
		};
	private:
		bool initialized = false;
		shared_ptr<Symbol> accessor;
		shared_ptr<ThreadedExpressionEvaluator<double> > evaluator;
		Scope* local_scope = nullptr;
		
		PluginParameter2<string, XMLValueReader, RequiredPolicy> raw_expression;
		PluginParameter2<string, XMLValueReader, RequiredPolicy> symbol;
		
		struct FunParameter {
			PluginParameter_Shared<string, XMLValueReader, RequiredPolicy> symbol;
			PluginParameter_Shared<string, XMLValueReader, OptionalPolicy> name;
			EvaluatorVariable::Type type;
		};
		vector<FunParameter> parameters;
		
};

class VectorFunction : public Plugin
{
	public:
		DECLARE_PLUGIN("VectorFunction");
		~VectorFunction() { if (accessor) const_cast<Scope*>(accessor->scope())->removeSymbol(accessor); }

		void loadFromXML(const XMLNode, Scope* scope) override;
		void init() { init(local_scope); }
		void init (const Scope* scope) override;
	
		string getExpr() const { return raw_expression(); };
		vector<string> getParams() const { 
			vector<string> names;
			
			std::transform (parameters.begin(), parameters.end(),back_inserter(names),
							[] (FunParameter const & param)	{
								return param.symbol();
							});
			return names;
		}
		bool isSpherical() const { return is_spherical(); }
		const string& getSymbol()  const { return symbol(); }
// 		Granularity getGranularity() const { if (evaluator) return evaluator->getGranularity(); else return Granularity::Global; };

		class Symbol: public SymbolAccessorBase<VDOUBLE> {
			public:
				Symbol(VectorFunction* parent) : SymbolAccessorBase<VDOUBLE>(parent->getSymbol()), parent(parent) {};
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
		shared_ptr<Symbol> accessor;      
		
		struct FunParameter {
			PluginParameter_Shared<string, XMLValueReader, RequiredPolicy> symbol;
			PluginParameter_Shared<string, XMLValueReader, OptionalPolicy> name;
			EvaluatorVariable::Type type;
		};
		vector<FunParameter> parameters;
};

#endif
