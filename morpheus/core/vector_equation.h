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
\defgroup ML_VectorEquation VectorEquation
\ingroup ML_Global
\ingroup ML_CellType
\ingroup MathExpressions

Assignment of mathematical expression to a vector symbol.

Syntax is comma-separated as given by \b notation :
  - orthogonal - x,y,z
  - radial     - r,φ,θ
  - or radial  - φ,θ,r

During simulation it is asserted that the provided relation always holds. Therefore, the expression may not depend on the referred output symbol. For recurrence equations, use a \ref ML_VectorRule within Systems.


\section Examples
Assign a value by comma separated list of 3 expressions.
\verbatim
<VectorEquation symbol-ref="v">
	<Expression> 2*a, a+1, a+b </Expression>
</VectorEquation>  
\endverbatim

Normalise a Vector to length of 5 using element-wise vector calculus (assume u is a Vector).
\verbatim
<VectorEquation symbol-ref="v">
	<Expression> 5 * u / u.abs </Expression>
</VectorEquation>  
\endverbatim

Using spherical coordinates
\verbatim
<VectorEquation symbol-ref="v" notation="r,φ,θ">
	<Expression> radius, angle_phi, angle_theta </Expression>
</VectorEquation>
\endverbatim
**/

/**
\defgroup ML_VectorRule VectorRule
\ingroup ML_System
\ingroup ML_Event
\ingroup MathExpressions

Assignment of mathematical expression to a symbol.

Differs from \ref ML_VectorEquation in that a VectorRule:
- may contain recurrence relations e.g. \f$ v = v.x, 2,0*v.y, v.z\f$
- can only appear in \ref ML_System
- explicitly scheduled based on user-specified System time-step

Syntax is comma-separated as given by \b notation :
	orthogonal - x,y,z
	radial     - r,φ,θ
	or radial  - φ,θ,r

\section Examples
Assign a value by comma separated list of 3 expressions.
\verbatim
<System solver="euler" time-step="1">
	<VectorRule symbol-ref="a">
		<Expression> 2*a, a+1, a+b </Expression>
	</VectorRule>  
</System>
\endverbatim
**/

#ifndef VECTOR_EQUATION_H
#define VECTOR_EQUATION_H

#include "core/interfaces.h"
#include "core/focusrange.h"


class VectorEquation : public ReporterPlugin {
	
	private:
		PluginParameter2<VDOUBLE,XMLWritableSymbol,RequiredPolicy> symbol;
		PluginParameter2<VDOUBLE, XMLThreadsaveEvaluator, RequiredPolicy> expression;
		PluginParameter2<VecNotation, XMLNamedValueReader, DefaultValPolicy> notation;
		
	public:
		DECLARE_PLUGIN("VectorEquation");

		VectorEquation();

		void report() override;
		
		string getExpr() { return expression.stringVal(); }
		string getSymbol() { return symbol.name(); };
		VecNotation getNotation() { return notation(); };

};

#endif
