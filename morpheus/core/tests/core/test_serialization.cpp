#include "test_operators.h"
#include "core/traits.h"
#include "core/delay.h"


TEST (SERIALIZATION, float) {
	float a = 1.0/3;
	EXPECT_PRED_FORMAT2(EQ_PREC,TypeInfo<float>::fromString(TypeInfo<float>::toString(a)),a);
	a = -1.0/3;
	EXPECT_PRED_FORMAT2(EQ_PREC,TypeInfo<float>::fromString(TypeInfo<float>::toString(a)),a);
	a = -1.0/3 * 1e-6;
	EXPECT_PRED_FORMAT2(EQ_PREC,TypeInfo<float>::fromString(TypeInfo<float>::toString(a)),a);
	
}

TEST (SERIALIZATION, double) {
	double a = 1.0/3;
	EXPECT_PRED_FORMAT2(EQ_PREC,TypeInfo<double>::fromString(TypeInfo<double>::toString(a)),a);
	a = -1.0/3 * 10e20;
	EXPECT_PRED_FORMAT2(EQ_PREC,TypeInfo<double>::fromString(TypeInfo<double>::toString(a)),a);
	a = -1.0/3 * 1e-10;
	EXPECT_PRED_FORMAT2(EQ_PREC,TypeInfo<double>::fromString(TypeInfo<double>::toString(a)),a);
}

TEST (SERIALIZATION, VDOUBLE) {
	VDOUBLE a (1.5, -2.5, 1.0/3);
	EXPECT_PRED_FORMAT2( EQ_PREC,TypeInfo<VDOUBLE>::fromString(TypeInfo<VDOUBLE>::toString(a)), a);
}

TEST (SERIALIZATION, VINT) {
	VINT a (1500, -32186998, 123584699);
	VINT b =TypeInfo<VINT>::fromString(TypeInfo<VINT>::toString(a));
	EXPECT_EQ(a.x,b.x);
	EXPECT_EQ(a.y,b.y);
	EXPECT_EQ(a.z,b.z);
}

TEST (SERIALIZATION, DEQUE) {
	deque<double> a;
	a.push_back(M_PI*1e12);
	a.push_back(0.00001569933342352352);
	a.push_back(1.0/7);
	a.push_back(-15133.15458569);
	
	auto b =TypeInfo<deque<double>>::fromString(TypeInfo<deque<double>>::toString(a));
	EXPECT_PRED_FORMAT2(EQ_PREC,a[0],b[0]);
	EXPECT_PRED_FORMAT2(EQ_PREC,a[1],b[1]);
	EXPECT_PRED_FORMAT2(EQ_PREC,a[2],b[2]);
	EXPECT_PRED_FORMAT2(EQ_PREC,a[3],b[3]);
}

TEST (SERIALIZATION, DELAY_BUFFER) {
	DelayBuffer a(3);
	
	a.push_back({-1,2});
	a.push_back({-0.5,1});
	a.push_back({-0, 0});
	
	auto b =TypeInfo<DelayBuffer>::fromString(TypeInfo<DelayBuffer>::toString(a));
	
	EXPECT_PRED_FORMAT2(EQ_PREC,a[0].value,b[0].value);
	EXPECT_PRED_FORMAT2(EQ_PREC,a[0].time,b[0].time);
	EXPECT_PRED_FORMAT2(EQ_PREC,a[1].value,b[1].value);
	EXPECT_PRED_FORMAT2(EQ_PREC,a[1].time,b[1].time);
	EXPECT_PRED_FORMAT2(EQ_PREC,a[2].value,b[2].value);
	EXPECT_PRED_FORMAT2(EQ_PREC,a[2].time,b[2].time);
}
