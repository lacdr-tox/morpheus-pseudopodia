
#include "gtest/gtest.h"
#include "model_test.h"
#include "core/simulation.h"

const double mass_error_tolerance_factor = 1e-8;
const double operator_error_tolerance_factor = 1.5e-3;

TEST (FieldDiffusion, Square) {
	
	auto file1 = ImportFile("field_diffusion_001.xml");
	auto model = TestModel(file1.getDataAsString());

	model.run();
	
	auto field = SIM::findGlobalSymbol<double>("f");
	
	auto initial_mass = SIM::findGlobalSymbol<double>("initial_mass") -> get(SymbolFocus::global);
	auto mass = SIM::findGlobalSymbol<double>("mass") -> get(SymbolFocus::global);
	EXPECT_NEAR(mass, initial_mass, initial_mass*mass_error_tolerance_factor);
	
	auto total_error = SIM::findGlobalSymbol<double>("total_error") -> get(SymbolFocus::global);
	EXPECT_NEAR(total_error/initial_mass, 0, operator_error_tolerance_factor);
}


TEST (FieldDiffusion, Hexagon) {
	
	auto file1 = ImportFile("field_diffusion_002.xml");
	auto model = TestModel(file1.getDataAsString());

	model.run();
	
	auto field = SIM::findGlobalSymbol<double>("f");
	
	auto initial_mass = SIM::findGlobalSymbol<double>("initial_mass") -> get(SymbolFocus::global);
	auto mass = SIM::findGlobalSymbol<double>("mass") -> get(SymbolFocus::global);
	EXPECT_NEAR(mass, initial_mass, initial_mass*mass_error_tolerance_factor);
	
	auto total_error = SIM::findGlobalSymbol<double>("total_error") -> get(SymbolFocus::global);
	EXPECT_NEAR(total_error/initial_mass, 0, operator_error_tolerance_factor);
}

TEST (FieldDiffusion, Cubic) {
	
	auto file1 = ImportFile("field_diffusion_003.xml");
	auto model = TestModel(file1.getDataAsString());

	model.run();
	
	auto field = SIM::findGlobalSymbol<double>("f");
	
	auto initial_mass = SIM::findGlobalSymbol<double>("initial_mass") -> get(SymbolFocus::global);
	auto mass = SIM::findGlobalSymbol<double>("mass") -> get(SymbolFocus::global);
	EXPECT_NEAR(mass, initial_mass, initial_mass*mass_error_tolerance_factor);
	
	auto total_error = SIM::findGlobalSymbol<double>("total_error") -> get(SymbolFocus::global);
	EXPECT_NEAR(total_error/initial_mass, 0, operator_error_tolerance_factor);
}
