#include "gtest/gtest.h"
#include "core/vec.h"

TEST (VEC, CONVERSION) {
	bool vint_to_vdouble = std::is_convertible<VINT, VDOUBLE>::value;
	bool vdouble_to_vint = std::is_convertible<VDOUBLE, VINT>::value;
	EXPECT_TRUE(vint_to_vdouble);
	EXPECT_FALSE(vdouble_to_vint);
}

// Automatic resolution of mismatching types in operators 
TEST (VDOUBLE, ADD) {
	EXPECT_EQ( VDOUBLE(1.5,1.5,1.5) + 1, VDOUBLE(2.5,2.5,2.5) );
	EXPECT_EQ( 1 + VDOUBLE(1.5,1.5,1.5), VDOUBLE(2.5,2.5,2.5) );
	VINT b(1,1,1);
	EXPECT_EQ( VDOUBLE(1.5,1.5,1.5) + b, VDOUBLE(2.5,2.5,2.5) );
	EXPECT_EQ( b + VDOUBLE(1.5,1.5,1.5), VDOUBLE(2.5,2.5,2.5) );
}

TEST (VDOUBLE, SUB) {
	EXPECT_EQ( VDOUBLE(1.5,1.5,1.5) - 1, VDOUBLE(0.5,0.5,0.5) );
	EXPECT_EQ( 2 - VDOUBLE(1.5,1.5,1.5), VDOUBLE(0.5,0.5,0.5) );
	VINT b(1,1,1);
	EXPECT_EQ( VDOUBLE(1.5,1.5,1.5) - b, VDOUBLE(0.5,0.5,0.5) );
	EXPECT_EQ( b - VDOUBLE(0.5,0.5,0.5), VDOUBLE(0.5,0.5,0.5) );
}


TEST (VDOUBLE, MULT) {
	EXPECT_EQ( VDOUBLE(1.5,1.5,1.5) * 2, VDOUBLE(3,3,3) );
	EXPECT_EQ( 2 * VDOUBLE(1.5,1.5,1.5), VDOUBLE(3,3,3) );
	VINT b(2,2,2);
	EXPECT_EQ( VDOUBLE(1.5,1.5,1.5) * 2, VDOUBLE(3,3,3) );
	EXPECT_EQ( 2 * VDOUBLE(1.5,1.5,1.5), VDOUBLE(3,3,3) );
}


TEST (VDOUBLE, DIV) {
	EXPECT_EQ( VDOUBLE(3,6,9) / 3, VDOUBLE(1,2,3) );
	EXPECT_EQ( VDOUBLE(3,6,9) / VINT(1,2,3), VDOUBLE(3,3,3) );
}

// Test the modulo operator to do periodic lattice modulo
TEST (VEC, MODULO) {
	EXPECT_EQ( VDOUBLE(11,12,13) % VINT(10,10,10), VDOUBLE(1,2,3) );
	EXPECT_EQ( VDOUBLE(-1,-2,-3) % VINT(10,10,10), VDOUBLE(9,8,7) );
	EXPECT_EQ( VDOUBLE(-10,-10,-10) % VINT(10,10,10), VDOUBLE(0,0,0) );
	EXPECT_EQ( VINT(11,12,13) % VINT(10,10,10), VINT(1,2,3) );
	EXPECT_EQ( VINT(-1,-2,-3) % VINT(10,10,10), VINT(9,8,7) );
	EXPECT_EQ( VINT(-10,-10,-10) % VINT(10,10,10), VINT(0,0,0) );
}

// Test the div operator to do periodic lattice modulo
TEST (VEC, DIV) {
	EXPECT_EQ( pdiv(VDOUBLE(11,12,13) , VINT(10,10,10)), VDOUBLE(1,1,1) );
	EXPECT_EQ( pdiv(VDOUBLE(-1,-2,-3) , VINT(10,10,10)), VDOUBLE(-1,-1,-1) );
	EXPECT_EQ( pdiv(VDOUBLE(-10,-10,-10) , VINT(10,10,10)), VDOUBLE(-1,-1,-1) );
	EXPECT_EQ( pdiv(VINT(11,12,13) , VINT(10,10,10)), VINT(1,1,1) );
	EXPECT_EQ( pdiv(VINT(-1,-2,-3) , VINT(10,10,10)), VINT(-1,-1,-1) );
	EXPECT_EQ( pdiv(VINT(-10,-10,-10) , VINT(10,10,10)), VINT(-1,-1,-1) );
}

// Test conversion from different notations
TEST (VDOUBLE, NOTATION) {
	EXPECT_EQ(VDOUBLE::from(VDOUBLE(1,2,3),VecNotation::ORTH), VDOUBLE(1,2,3));
	EXPECT_EQ(VDOUBLE::from(VDOUBLE(1,0.5*M_PI,0),VecNotation::SPHERE_RPT), VDOUBLE(0,0,1));
	EXPECT_EQ(VDOUBLE::from(VDOUBLE(2,0,0.5*M_PI),VecNotation::SPHERE_RPT), VDOUBLE(0,2,0));
	EXPECT_EQ(VDOUBLE::from(VDOUBLE(0,-0.5*M_PI,1),VecNotation::SPHERE_PTR), VDOUBLE(0,0,-1));
	EXPECT_EQ(VDOUBLE::from(VDOUBLE(-0.5*M_PI,0,2),VecNotation::SPHERE_PTR), VDOUBLE(0,-2,0));
}


#ifdef HAVE_VEC_INDEX_OPRT
TEST (VDOUBLE, IDX_OPERATOR) {
	EXPECT_EQ( VDOUBLE(1.5,2.5,3.5)[0] , 1.5 );
	EXPECT_EQ( VDOUBLE(1.5,2.5,3.5)[1] , 2.5 );
	EXPECT_EQ( VDOUBLE(1.5,2.5,3.5)[2] , 3.5 );
	EXPECT_EQ( VINT(1,2,3)[0] , 1 );
	EXPECT_EQ( VINT(1,2,3)[1] , 2 );
	EXPECT_EQ( VINT(1,2,3)[2] , 3 );
}
#endif

