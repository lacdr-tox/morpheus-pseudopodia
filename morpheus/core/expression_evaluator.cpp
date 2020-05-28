#include "expression_evaluator.h"
#include "simulation.h"
#include "boost/math/special_functions/factorials.hpp"

template <>
int ExpressionEvaluator<double>::expectedNumResults() const { return 1; }

template <>
double ExpressionEvaluator<double>::get(const SymbolFocus& focus, bool safe) const
{
	if (safe && !initialized)
		const_cast<ExpressionEvaluator*>(this)->init();
	if (expr_is_const)
		return const_val;
	if (expr_is_symbol)
		return safe || allow_partial_spec ? symbol_val->safe_get(focus) : symbol_val->get(focus);
	
	evaluator_cache->fetch(focus, safe || allow_partial_spec);
	
	if (delay_const_expr_init) {
		const_val = parser->Eval((void*)&focus);
		expr_is_const = true;
		return const_val;
	}
	return parser->Eval((void*)&focus);
}

template <>
double ExpressionEvaluator<double>::plain_get(const SymbolFocus& focus) const
{
	if (expr_is_const)
		return const_val;
	
	if (delay_const_expr_init) {
		const_val = parser->Eval((void*)&focus);
		expr_is_const = true;
		return const_val;
	}
	return parser->Eval((void*)&focus);
}


template <>
int ExpressionEvaluator<float>::expectedNumResults() const { return 1; }

template <>
float ExpressionEvaluator<float>::get(const SymbolFocus& focus, bool safe) const
{
	if (safe && !initialized)
			const_cast<ExpressionEvaluator*>(this)->init();
	if (expr_is_const)
		return const_val;;
	
	if (expr_is_symbol)
		return safe || allow_partial_spec ? symbol_val->safe_get(focus) : symbol_val->get(focus);
	
	evaluator_cache->fetch(focus, safe || allow_partial_spec);
	
	if (delay_const_expr_init) {
		const_val = parser->Eval((void*)&focus);
		expr_is_const = true;
		return const_val;
	}
	return parser->Eval((void*)&focus);
}

template <>
float ExpressionEvaluator<float>::plain_get(const SymbolFocus& focus) const
{
	if (expr_is_const)
		return const_val;
	
	if (delay_const_expr_init) {
		const_val = parser->Eval((void*)&focus);
		expr_is_const = true;
		return const_val;
	}
	return parser->Eval((void*)&focus);
}

template <>
int ExpressionEvaluator<VDOUBLE>::expectedNumResults() const { return 3; }


template <>
VDOUBLE ExpressionEvaluator<VDOUBLE>::get(const SymbolFocus& focus, bool safe) const
{
	if (safe && !initialized)
		const_cast<ExpressionEvaluator*>(this)->init();
	
	if (expr_is_const)
		return const_val;
	
	if (expr_is_symbol)
		VDOUBLE::from(symbol_val->get(focus), _notation);
	
	VDOUBLE result;
	evaluator_cache->fetch(focus, safe | allow_partial_spec);
	if (expand_scalar_expr) {
		evaluator_cache->setExpansionIndex(0);
		result.x = parser->Eval((void*)&focus);
		
		evaluator_cache->setExpansionIndex(1);
		result.y = parser->Eval((void*)&focus);
		
		evaluator_cache->setExpansionIndex(2);
		result.z = parser->Eval((void*)&focus);
	}
	else {
		int n;
		double* results = parser->Eval(n,(void*)&focus);
		if (n != expectedNumResults()) {
			cerr << "Wrong number of results " << n << " of 3 in VectorExpression " << this->expression << endl;
			throw string("Wrong number of expressions in VectorExpression ") + this->expression;
		}
		result = VDOUBLE::from(VDOUBLE(results[0],results[1],results[2]), _notation);
	}
	if (delay_const_expr_init) {
		const_val = result;
		expr_is_const = true;
		return const_val;
	}
	return result;
}

template <>
VDOUBLE ExpressionEvaluator<VDOUBLE>::plain_get(const SymbolFocus& focus) const
{
	if (expr_is_const)
		return const_val;
	
	VDOUBLE result;
	if (expand_scalar_expr) {
		evaluator_cache->setExpansionIndex(0);
		result.x = parser->Eval((void*)&focus);
		
		evaluator_cache->setExpansionIndex(1);
		result.y = parser->Eval((void*)&focus);
		
		evaluator_cache->setExpansionIndex(2);
		result.z = parser->Eval((void*)&focus);
	}
	else {
		int n;
		double* results = parser->Eval(n,(void*)&focus);
		if (n != expectedNumResults()) {
			cerr << "Wrong number of results " << n << " of 3 in VectorExpression " << this->expression << endl;
			throw string("Wrong number of expressions in VectorExpression ") + this->expression;
		}
		result = VDOUBLE::from(VDOUBLE(results[0],results[1],results[2]), _notation);
	}
	if (delay_const_expr_init) {
		const_val = result;
		expr_is_const = true;
		return const_val;
	}
	return result;
}

/* Implementation of math functions registered in the parser */

double unary_plus(double val) {
	return val;
}

double getPlusMulti(const double *a, int cnt) {
	double b=0; int i=0;
	while (i<cnt) { b+=a[i]; ++i; }
	return b;
}

double getTimesMulti(const double *a, int cnt) {
	double b=1; int i=0;
	while (i<cnt) { b*=a[i]; ++i; }
	return b;
}

double getRandomUniValue(double min, double max) {
        return getRandom01()*(max-min)+min;
}
double getRandomIntValue(double min, double max) {
	return floor(getRandom01()*(max+1-min)+min);
}
double getRandomNormValue(double mean, double stdev) {
        return mean+getRandomGauss(stdev);
}
double getRandomBoolValue() {
        return getRandomBool();
}
double getRandomGammaValue(double shape, double scale){
        return getRandomGamma(shape, scale);
}
double getModulo(double a, double b) {
	return fmod(a,b);
}

double getPow(double base, double power) {
	return pow(base,power);
}

double getIf(double cond, double true_case, double false_case) {
	return cond ? true_case : false_case;
}

double getAND(double left, double right) {
	return left && right;
}

double getANDmulti(const double *d, int cnt) {
	if (!cnt) return 1;
	uint i=0;
	bool r = d[i];
	while (++i<cnt) r&= d[i]>0;
	return r;
}

double getOR(double left, double right) {
	return left || right;
}

double getORmulti(const double *d, int cnt) {
	bool r=0; uint i=0;
	while (i<cnt) { r|= d[i]>0; ++i; }
	return r;
}

double getXOR(double left, double right) {
	return !left xor !right;
}

double getXORmulti(const double *d, int cnt) {
	bool r=0; int i=0;
	while (i<cnt) { r= r xor (d[i]>0); i++; }
	return r;
}

double getNOT(double val) {
	return val<=0;
}

double getLEQmulti(const double* a, int cnt) {
	bool r = true; int i=1;
	while (i<cnt) { r &= a[i-1]<=a[i]; i++; }
	return r;
}

double getGEQmulti(const double* a, int cnt) {
	bool r = true; int i=1;
	while (i<cnt) { r &= a[i-1]>=a[i]; i++; }
	return r;
}

double getLTmulti(const double* a, int cnt) {
	bool r = true; int i=1;
	while (i<cnt) { r &= a[i-1]<a[i]; i++; }
	return r;
}

double getGTmulti(const double* a, int cnt) {
	bool r = true; int i=1;
	while (i<cnt) { r &= a[i-1]>a[i]; i++; }
	return r;
}

double getEQ(const double* a, int cnt) {
	bool r = true; int i=1;
	while (i<cnt) { r &= a[i-1]==a[i]; i++; }
	return r;
}

double getNEQ(const double* a, int cnt) {
	bool r = true; int i=1;
	while (i<cnt) { r &= a[i-1]!=a[i]; i++; }
	return r;
}

double piecewise_3_function(double v0, double c0, double velse) {
	return c0 ? v0 : velse;
}

double piecewise_5_function(double v0, double c0, double v1, double c1,double velse) {
	return c0 ? v0 : (c1 ? v1 :velse);
}

double piecewiseMulti(const double* a, int cnt) {
	int i=1;
	while (cnt>i && (a[i]<=0)) i+=2;
	if (i>=cnt) {
		// Pick otherwise
		if (i==cnt) return a[i-1];
		// Return default for missing otherwise
		else return 0;
	}
	else
		// Condition at i is true
		return a[i-1];
}

double getImplies(double a, double b) {
	return !bool(a) || bool(b);
}

double getLog(double a) { return log(a); }
double getCeil(double a) { return ceil(a); } 
double getFloor(double a) { return  floor(a + 1e-12); }
double getRem(double a, double b) { return (a-floor(a/b + 1e-12)*b);}
double getQuotient(double a, double b) { return floor(a/b + 1e-12);}
double getFactorial(double a) { return boost::math::factorial<double>(uint(a)); }
double getCot(double a) { return 1.0/tan(a); }
double getACot(double a) { return atan(1.0/a); }
double getCotH(double a) { return 1.0/tanh(a); }
double getACotH(double a) { return atanh(1.0/a); }
double getSec(double a) { return 1.0/cos(a); }
double getASec(double a) { return acos(1.0/a); }
double getSecH(double a) { return 1.0/cosh(a); }
double getASecH(double a) { return acosh(1.0/a); }
double getCsc(double a) { return 1.0/sin(a); }
double getACsc(double a) { return asin(1.0/a); }
double getCscH(double a) { return 1.0/sinh(a); }
double getACscH(double a) {return asinh(1.0/a); }

unique_ptr< mu::Parser > createMuParserInstance()
{
	unique_ptr<mu::Parser> parser(new mu::Parser());
	try {
		string name_chars = "0123456789_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZαβγδεζηθικλμνξοπρσςτυφχψωΑΒΓΔΕΖΗΘΙΚΛΜΝΞΟΠΡΣΤΥΦΧΨΩ.";
		parser->DefineNameChars(name_chars.c_str());

		// These constants might also move to a super-global scope, such that they may be overridden.
		parser->DefineConst("false",		0);
		parser->DefineConst("true",			1);
		parser->DefineConst("pi",			M_PI);
		parser->DefineConst("exponentiale", M_E );
		parser->DefineConst("avogadro",		6.02214179e23);
		
		parser->DefineInfixOprt("+",		&unary_plus);
		
		parser->DefineFun( "plus",			&getPlusMulti,  true);
		parser->DefineFun( "times",			&getTimesMulti,  true);
		
		parser->DefineFun( sym_RandomUni,	&getRandomUniValue,  false);
		parser->DefineFun( sym_RandomInt,	&getRandomIntValue,  false);
		parser->DefineFun( sym_RandomNorm,	&getRandomNormValue, false);
		parser->DefineFun( sym_RandomBool,	&getRandomBoolValue, false);
		parser->DefineFun( sym_RandomGamma, &getRandomGammaValue, false);
		parser->DefineFun( sym_Modulo, 		&getModulo, true);

// logicals
		parser->DefineFun( "if",			&getIf, true);
		parser->DefineFun("piecewise", 		&piecewiseMulti, true);
		parser->DefineInfixOprt("!",		&getNOT, mu::prINFIX, true);
		parser->DefineOprt("and",			&getAND, mu::prLAND, mu::oaLEFT, true);
		parser->DefineOprt("or",			&getOR, mu::prLOR, mu::oaLEFT, true);
		parser->DefineOprt("xor",			&getXOR, mu::prLOR, mu::oaLEFT, true);

		parser->DefineFun("leq",			&getLEQmulti, true);
		parser->DefineFun("geq",			&getGEQmulti, true);
		parser->DefineFun("lt",				&getLTmulti, true);
		parser->DefineFun("gt",				&getGTmulti, true);
		parser->DefineFun("eq",				&getEQ, true);
		parser->DefineFun("neq",			&getNEQ, true);
		
		parser->DefineFun("and_f",		&getANDmulti, true);
		parser->DefineFun("or_f",			&getORmulti, true);
		parser->DefineFun("xor_f",		&getXORmulti, true);
		parser->DefineFun("not_f",		&getNOT, true);
		parser->DefineFun("implies",		&getImplies, true);
		
// SBML Import compatibility (from MathML <piecewise> construct)
		parser->DefineFun("ceil",			&getCeil, true);
		parser->DefineFun("floor",			&getFloor, true);
		parser->DefineFun("factorial",		&getFactorial, true);
		parser->DefineFun("rem",			&getRem, true);
		parser->DefineFun("quotient",		&getQuotient, true);
		parser->DefineFun("root",			&mu::MathImpl<double>::Sqrt, true);
		
		parser->DefineFun("pow",			&getPow, true);
		parser->DefineFun("log",			&getLog, true);
		
		parser->DefineFun("cot", 			getCot, true);
		parser->DefineFun("acot", 			getACot, true);
		parser->DefineFun("arccot", 		getACot, true);
		parser->DefineFun("coth", 			getCot, true);
		parser->DefineFun("acoth", 			getACotH, true);
		parser->DefineFun("arccoth", 		getACotH, true);
		parser->DefineFun("sec", 			getSec, true);
		parser->DefineFun("asec", 			getASec, true);
		parser->DefineFun("arcsec", 		getASec, true);
		parser->DefineFun("sech", 			getSecH, true);
		parser->DefineFun("asech", 			getASecH, true);
		parser->DefineFun("arcsech", 		getASecH, true);
		parser->DefineFun("csc", 			getCsc, true);
		parser->DefineFun("acsc", 			getACsc, true);
		parser->DefineFun("arccsc",			getACsc, true);
		parser->DefineFun("csch", 			getCscH, true);
		parser->DefineFun("acsch", 			getACscH, true);
		parser->DefineFun("arccsch", 		getACscH, true);
		parser->DefineFun("arcsin",			&mu::MathImpl<double>::ASin, true);
		parser->DefineFun("arccos",			&mu::MathImpl<double>::ACos, true);
		parser->DefineFun("arctan",			&mu::MathImpl<double>::ATan, true);
		parser->DefineFun("arcsinh",		&mu::MathImpl<double>::ASinh, true);
		parser->DefineFun("arccosh",		&mu::MathImpl<double>::ACosh, true);
		parser->DefineFun("arctanh",		&mu::MathImpl<double>::ATanh, true);
		
		
	}
		
	catch (mu::ParserError& e) {
		throw e.GetMsg() + " in muParser instance creation.";
	}
	return parser;
	
}

