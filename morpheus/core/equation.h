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
\defgroup Equation
\ingroup MathExpressions

Assignment of mathematical expression to a symbol.

During simulation it is asserted that the provided relation always holds. The assignment is executed whenever the input might have changed and/or the result is needed elsewhere.

For vector data, use \ref VectorEquation.

For recurrence equations (in which the expression depends on the output symbol), use a \ref Rule within a \ref System.

\section Example
Assume 'a' is a variable or property.
\verbatim
<Equation symbol-ref="a">
	(u*v)/(1+v)
</Equation>  
\endverbatim

**/

/**
\defgroup Rule
\ingroup MathExpressions

Assignment of mathematical expression to a symbol.

Differs from \ref Equation in that a Rule:
- may contain recurrence relations e.g. \f$ A=A+1 \f$
- can only appear in \ref System
- explicitly scheduled based on user-specified System time-step

\section Examples
Assign a new value to 'a' based on values of 'a' in previous time-step.
\verbatim
<System solver="euler" time-step="1">
	<Constant symbol="n" value="2">
	<Rule symbol-ref="a">
		<Expression> a^n / (a^n+1) </Expression>
	</Rule>  
</System>
\endverbatim
**/


#ifndef EQUATION_H
#define EQUATION_H

#include "expression_evaluator.h"
#include "symbol_accessor.h"
#include "interfaces.h"
#include "focusrange.h"
#include "plugin_parameter.h"

class Equation : public ReporterPlugin
{
	private:
		shared_ptr<ThreadedExpressionEvaluator<double> > evaluators;
		string expression;
		PluginParameter2<double,XMLReadWriteSymbol,RequiredPolicy> symbol;

	public:
		DECLARE_PLUGIN("Equation");

		Equation();
		virtual void loadFromXML(const XMLNode );

		virtual void report();
		
		virtual void init(const Scope* scope);
		string getExpr() { return expression; }
		string getSymbol() { return symbol.name(); };
};

#endif // EQUATION_H
