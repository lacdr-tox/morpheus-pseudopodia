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
\ingroup MathExpressions
\ingroup Symbols

Symbol that defines a relation between \ref Symbols. 

Functions are not explicitly scheduled. Instead they are evaluated 'on-the-fly' whenever their output symbols are used.


For vector data, use \ref VectorFunction.

To assign to a variable or property, use \ref Equation.

\section Example
Assume 'a' is a variable or property.
\verbatim
<Function symbol="a">
	(u*v)/(1+v)
</Function>  
\endverbatim
**/

#ifndef Function_H
#define Function_H
#include "core/interfaces.h"
#include "expression_evaluator.h"


class Function : public ReporterPlugin {
	private:
		string raw_expression;
		shared_ptr<ThreadedExpressionEvaluator<double> > evaluator;
		
		string function_symbol;
		string function_fullname;

	public:
		DECLARE_PLUGIN("Function");
		Function();
		
		virtual XMLNode saveToXML() const;
		void loadFromXML(const XMLNode) override;
		
// 		static shared_ptr<mu::Parser> createParserInstance(); 
// 		void setExpr(string expression);
		string getExpr() const;

		void init (const Scope* scope) override;
		void report() override {};

		double get(const CPM::CELL_ID cell_id) const;
		double get(const VINT& pos) const;
		double get(const CPM::CELL_ID cell_id, const VINT& pos) const;
		double get(const SymbolFocus& focus) const;
		const string& getSymbol()  const { return function_symbol; }
		Granularity getGranularity() const;
};

class VectorFunction : public ReporterPlugin
{
	private:
		string raw_expression;
		bool is_spherical;
		
		shared_ptr<ExpressionEvaluator<VDOUBLE> > evaluator;
		
		string function_symbol;
		string function_fullname;

	public:
		DECLARE_PLUGIN("VectorFunction");
		VectorFunction();

		void loadFromXML(const XMLNode) override;
	
		string getExpr() const;
		
		bool isSpherical() const { return is_spherical; }

		void init (const Scope* scope) override;
		void report() override {};

		VDOUBLE get(const SymbolFocus& focus) const;
		const string& getSymbol()  const { return function_symbol; }
		Granularity getGranularity() const;
};

namespace SIM {
	void defineSymbol(shared_ptr<Function> f);
	void defineSymbol(shared_ptr<VectorFunction> f);
}

#endif
