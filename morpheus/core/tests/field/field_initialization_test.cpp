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
	EXPECT_EQ(mean,0.5);
	EXPECT_EQ(mass,2);
	
	model.setParam("s", "5");
	model.run();
	auto s = SIM::findGlobalSymbol<double>("s") -> get(SymbolFocus::global);
	auto size = SIM::findGlobalSymbol<VDOUBLE>("size") -> get(SymbolFocus::global);
	mean = SIM::findGlobalSymbol<double>("mean") -> get(SymbolFocus::global);
	mass = SIM::findGlobalSymbol<double>("mass") -> get(SymbolFocus::global);
	EXPECT_EQ(s, 5);
	EXPECT_EQ(size,VDOUBLE(5,5,0));
	EXPECT_EQ(mean,0.5);
	EXPECT_EQ(mass,0.5 * 5 * 5);
}
