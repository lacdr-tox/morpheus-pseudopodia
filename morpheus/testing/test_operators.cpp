#include "test_operators.h"

/** Test precision of data serialization **/
using std::string;
const double abs_prec = 1e-15;
const double rel_prec = 1e-10;
const string rel_prec_string = "1e-10";
const double abs_prec_float = 1e-9;
const double rel_prec_float = 1e-5;
const string rel_prec_float_string = "1e-5";


::testing::AssertionResult EQ_PREC(const char* m_expr, const char* n_expr, double m, double n) {
	double d = m-n; auto d_err = rel_prec*abs(n) + abs_prec;
	if (d < d_err  && d > -d_err)
		return ::testing::AssertionSuccess();
	else
		return ::testing::AssertionFailure() << "Error in Serialization " << m_expr << "\n" << "Relative error of '" << m << "' and '" << n << "' is above " << rel_prec_string;
}

::testing::AssertionResult EQ_PREC(const char* m_expr, const char* n_expr, float  m, float n) {
	float d = m-n; auto d_err = rel_prec_float*abs(n) + abs_prec_float;
	if (d < d_err  && d > -d_err)
		return ::testing::AssertionSuccess();
	else
		return ::testing::AssertionFailure() << "Error in Serialization " << m_expr << "\n" << "Relative error of '" << m << "' and '" << n << "' is above " << rel_prec_float_string;
}

::testing::AssertionResult EQ_PREC(const char* m_expr, const char* n_expr, VDOUBLE m, VDOUBLE n) {
	bool failed=false;
	
	double d = m.x-n.x; 
	auto d_err = rel_prec*abs(n.x) + abs_prec;
	if ( ! ( d < d_err && d > -d_err )) failed=true;
	d = m.y-n.y;
	d_err = rel_prec*abs(n.y) + abs_prec;
	if ( ! ( d < d_err && d > -d_err )) failed=true;
	d = m.z-n.z;
	d_err = rel_prec*abs(n.z) + abs_prec;
	if ( ! ( d < d_err && d > -d_err )) failed=true;

	if (failed)
		return ::testing::AssertionFailure() << "Error in Serialization " << m_expr << "\n"  << "Relative error of '" << m << "' and '" << n << "' is above " << rel_prec_string << "\n" << TypeInfo<VDOUBLE>::toString(m-n);
	else 
		return ::testing::AssertionSuccess();
}
