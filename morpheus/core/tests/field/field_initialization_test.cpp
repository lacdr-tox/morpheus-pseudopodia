#include "gtest/gtest.h"
#include "model_test.h"
#include "core/simulation.h"

TEST (FieldInitialisation, Spatial) {
	
	auto file1 = ImportFile("field_initialization_001.xml");
	auto model = TestModel(file1.getDataAsString());
	
	model.setParam("s", "2");
	model.run();
	
	auto mean = SIM::findGlobalSymbol<double>("mean") -> get(SymbolFocus::global);
	auto mass = SIM::findGlobalSymbol<double>("mass") -> get(SymbolFocus::global);
	EXPECT_DOUBLE_EQ(mean,0.5);
	EXPECT_DOUBLE_EQ(mass,2);
	
	model.setParam("s", "5");
	model.run();
	auto s = SIM::findGlobalSymbol<double>("s") -> get(SymbolFocus::global);
	auto size = SIM::findGlobalSymbol<VDOUBLE>("size") -> get(SymbolFocus::global);
	mean = SIM::findGlobalSymbol<double>("mean") -> get(SymbolFocus::global);
	mass = SIM::findGlobalSymbol<double>("mass") -> get(SymbolFocus::global);
	EXPECT_DOUBLE_EQ(s, 5);
	EXPECT_EQ(size,VDOUBLE(5,5,0));
	EXPECT_DOUBLE_EQ(mean,0.5);
	EXPECT_DOUBLE_EQ(mass,0.5 * 5 * 5);
}

TEST (FieldInitialisation, PeriodicBoundaries) {
	
	auto file1 = ImportFile("field_periodic_boundary.xml");
	auto model = TestModel(file1.getDataAsString());
	
	model.run();
	
	auto average = SIM::findGlobalSymbol<double>("a_a") -> get(SymbolFocus::global);
	auto sum = SIM::findGlobalSymbol<double>("a_s") -> get(SymbolFocus::global);
	EXPECT_DOUBLE_EQ(average,6.0);
	EXPECT_DOUBLE_EQ(sum,36.0);
}

TEST (FieldInitialisation, ConstantBoundaries) {
	auto file1 = ImportFile("field_const_grad_boundary.xml");
	auto model = TestModel(file1.getDataAsString());
	model.run();
	auto average = SIM::findGlobalSymbol<double>("a_a") -> get(SymbolFocus::global);
	auto sum = SIM::findGlobalSymbol<double>("a_s") -> get(SymbolFocus::global);
	EXPECT_DOUBLE_EQ(average,1.0);
	EXPECT_DOUBLE_EQ(sum,6.0);
}
