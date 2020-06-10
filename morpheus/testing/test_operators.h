#include "gtest/gtest.h"
#include "core/vec.h"
#include <string>

::testing::AssertionResult EQ_PREC(const char* m_expr, const char* n_expr, double m, double n);

::testing::AssertionResult EQ_PREC(const char* m_expr, const char* n_expr, float  m, float n);

::testing::AssertionResult EQ_PREC(const char* m_expr, const char* n_expr, VDOUBLE m, VDOUBLE n);
