#include "expression_evaluator.h"
#include "simulation.h"

template <>
int ExpressionEvaluator<double>::expectedNumResults() const { return 1; }

template <>
double ExpressionEvaluator<double>::get(const SymbolFocus& focus, bool safe) const
{
	if (expr_is_const)
		return const_val;
	if (expr_is_symbol)
		return safe ? symbols.front()->safe_get(focus) : symbols.front()->get(focus);
	uint i=0;
	try {
		
		for (i = 0; i<symbols.size(); i++) {
			symbol_values[i] = safe ? symbols[i]->safe_get(focus) : symbols[i]->get(focus);
		}
	} 
	catch (string e) {
		e+= "\nIn expression '" + this->getExpression() + "' for symbol '" + symbols[i]->name() +"'.";
		throw e;
	}
	return parser->Eval();
}




template <>
int ExpressionEvaluator<float>::expectedNumResults() const { return 1; }

template <>
float ExpressionEvaluator<float>::get(const SymbolFocus& focus, bool safe) const
{
	if (expr_is_const)
		return const_val;;
	
	if (expr_is_symbol)
		return safe ? symbols.front()->safe_get(focus) : symbols.front()->get(focus);
	
	uint i = 0;
	try {
		for (i = 0; i<symbols.size(); i++) {
			symbol_values[i] = safe ? symbols[i]->safe_get(focus) : symbols[i]->get(focus);
		}
	} 
	catch (string e) {
		e+= "\n In expression '" + this->getExpression() + "' for symbol '" + symbols[i]->name() +"'.";
		throw e;
	}
	
	return parser->Eval();
}

template <>
int ExpressionEvaluator<VDOUBLE>::expectedNumResults() const { return 3; }

template <>
const string& ExpressionEvaluator<VDOUBLE>::getDescription() const
{
	if (expr_is_symbol)
		return v_symbols[0]->description();
	else 
		return expression;
}

template <>
VDOUBLE ExpressionEvaluator<VDOUBLE>::get(const SymbolFocus& focus, bool safe) const
{
	if (expr_is_const)
		return const_val;
	
	if (expr_is_symbol)
		return v_symbols.front()->get(focus);
	
	VDOUBLE result;
	if (expand_scalar_expr) {
		for (uint i = 0; i<symbols.size(); i++) {
			symbol_values[i] = safe ? symbols[i]->safe_get(focus) : symbols[i]->get(focus);
		}
		
		for (uint i=0;i<v_symbols.size(); i++) {
			symbol_values[v_sym_cache_offset+i] = safe ? v_symbols[i]->safe_get(focus).x : v_symbols[i]->get(focus).x;
		}
		result.x = parser->Eval();
		
		for (uint i=0;i<v_symbols.size(); i++) {
			symbol_values[v_sym_cache_offset+i] = safe ? v_symbols[i]->safe_get(focus).y : v_symbols[i]->get(focus).y;
		}
		result.y = parser->Eval();
		
		for (uint i=0;i<v_symbols.size(); i++) {
			symbol_values[v_sym_cache_offset+i] = safe ? v_symbols[i]->safe_get(focus).z : v_symbols[i]->get(focus).z;
		}
		result.z = parser->Eval();
	}
	else {
		for (uint i = 0; i<symbols.size(); i++) {
			symbol_values[i] = safe ? symbols[i]->safe_get(focus) : symbols[i]->get(focus);
		}

		int n;
		double* results = parser->Eval(n);
		if (n != expectedNumResults()) {
			cerr << "Wrong number of results " << n << " of 3 in VectorExpression " << this->expression << endl;
			throw string("Wrong number of expressions in VectorExpression ") + this->expression;
		}
		result = VDOUBLE(results[0],results[1],results[2]);
	}
	return result;
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

double piecewise_3_function(double v0, double c0, double velse) {
	return c0 ? v0 : velse;
}

double piecewise_5_function(double v0, double c0, double v1, double c1,double velse) {
	return c0 ? v0 : (c1 ? v1 :velse);
}

unique_ptr< mu::Parser > createMuParserInstance()
{
	unique_ptr<mu::Parser> parser(new mu::Parser());
	string name_chars = "0123456789_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZαβγδεζηθικλμνξοπρσςτυφχψωΑΒΓΔΕΖΗΘΙΚΛΜΝΞΟΠΡΣΤΥΦΧΨΩ.";
	parser->DefineNameChars(name_chars.c_str());
	parser->DefineConst("pi",M_PI);
	parser->DefineFun( sym_RandomUni,	&getRandomUniValue,  false);
	parser->DefineFun( sym_RandomInt,	&getRandomIntValue,  false);
	parser->DefineFun( sym_RandomNorm,	&getRandomNormValue, false);
	parser->DefineFun( sym_RandomBool,	&getRandomBoolValue, false);
	parser->DefineFun( sym_RandomGamma, &getRandomGammaValue, false);
	parser->DefineFun( sym_Modulo, 		&getModulo, true);
	parser->DefineFun( "pow",			&getPow, true);
	parser->DefineFun( "if",			&getIf, true);
	parser->DefineOprt("and",			&getAND, 1, mu::oaLEFT, true);
	parser->DefineOprt("or",			&getOR, 2, mu::oaLEFT, true);
	// SBML Import compatibility (from MathML <piecewise> construct)
	parser->DefineFun("piecewise", &piecewise_3_function, true);
	parser->DefineFun("piecewise", &piecewise_5_function, true);
	
	return parser;
	
}
