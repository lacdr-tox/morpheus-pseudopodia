#include "expression_evaluator.h"
#include "simulation.h"

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
	return parser->Eval((void*)&focus);
}

template <>
double ExpressionEvaluator<double>::plain_get(const SymbolFocus& focus) const
{
	if (expr_is_const)
		return const_val;
	
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
	return parser->Eval((void*)&focus);
}

template <>
float ExpressionEvaluator<float>::plain_get(const SymbolFocus& focus) const
{
	if (expr_is_const)
		return const_val;
	
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
		return symbol_val->get(focus);
	
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
		result = VDOUBLE(results[0],results[1],results[2]);
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
		result = VDOUBLE(results[0],results[1],results[2]);
	}
	return result;
}

double unary_plus(double val) {
	return val;
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

double getOR(double left, double right) {
	return left || right;
}

double getXOR(double left, double right) {
	return !left xor !right;
}

double getNOT(double val) {
	return val<=0;
}

double getLEQ(double left, double right) {
	return left <= right;
}

double getGEQ(double left, double right) {
	return left >= right;
}

double getLT(double left, double right) {
	return left < right;
}

double getGT(double left, double right) {
	return left > right;
}

double getEQ(double left, double right) {
	return left == right;
}

double getNEQ(double left, double right) {
	return left != right;
}

double piecewise_3_function(double v0, double c0, double velse) {
	return c0 ? v0 : velse;
}

double piecewise_5_function(double v0, double c0, double v1, double c1,double velse) {
	return c0 ? v0 : (c1 ? v1 :velse);
}

unique_ptr< mu::Parser > createMuParserInstance()
{
	unique_ptr<mu::Parser> parser(new mu::Parser());
	try {
		string name_chars = "0123456789_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZαβγδεζηθικλμνξοπρσςτυφχψωΑΒΓΔΕΖΗΘΙΚΛΜΝΞΟΠΡΣΤΥΦΧΨΩ.";
		parser->DefineNameChars(name_chars.c_str());
		parser->DefineConst("pi",M_PI);
		parser->DefineInfixOprt("+",		&unary_plus);
		parser->DefineFun( sym_RandomUni,	&getRandomUniValue,  false);
		parser->DefineFun( sym_RandomInt,	&getRandomIntValue,  false);
		parser->DefineFun( sym_RandomNorm,	&getRandomNormValue, false);
		parser->DefineFun( sym_RandomBool,	&getRandomBoolValue, false);
		parser->DefineFun( sym_RandomGamma, &getRandomGammaValue, false);
		parser->DefineFun( sym_Modulo, 		&getModulo, true);
		parser->DefineFun( "pow",			&getPow, true);
		parser->DefineFun( "if",			&getIf, true);
		parser->DefineInfixOprt("!",		&getNOT, mu::prINFIX, true);
		parser->DefineOprt("and",			&getAND, mu::prLAND, mu::oaLEFT, true);
		parser->DefineOprt("or",			&getOR, mu::prLOR, mu::oaLEFT, true);
		parser->DefineOprt("xor",			&getXOR, mu::prLOR, mu::oaLEFT, true);
		
		// SBML Import compatibility (from MathML <piecewise> construct)
		parser->DefineFun("piecewise", 		&piecewise_3_function, true);
		parser->DefineFun("piecewise", 		&piecewise_5_function, true);
		parser->DefineFun("leq",			&getLEQ, true);
		parser->DefineFun("geq",			&getGEQ, true);
		parser->DefineFun("lt",				&getLT, true);
		parser->DefineFun("gt",				&getGT, true);
		parser->DefineFun("eq",				&getEQ, true);
		parser->DefineFun("neq",			&getNEQ, true);
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

