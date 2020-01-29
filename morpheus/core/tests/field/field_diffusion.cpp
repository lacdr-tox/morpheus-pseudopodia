
#include "gtest/gtest.h"
#include "model_test.h"
#include "core/simulation.h"

TEST (FieldDiffusion, Square) {
	
	auto file1 = ImportFile("field_diffusion_001.xml");
	auto model = TestModel(file1.getDataAsString());

	model.run();
	
	auto field = SIM::findGlobalSymbol<double>("f");
	
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
