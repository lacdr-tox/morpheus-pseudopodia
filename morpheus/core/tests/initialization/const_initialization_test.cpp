#include "gtest/gtest.h"
#include "model_test.h"
#include "core/simulation.h"

TEST (ConstInit, Function) {
	
	auto file1 = ImportFile("const_function.xml");
	auto model = TestModel(file1.getDataAsString());

	model.run();
	auto result = SIM::findGlobalSymbol<double>("result");
	EXPECT_TRUE(result->flags().space_const);
	EXPECT_TRUE(result->flags().time_const);
	auto solution = SIM::findGlobalSymbol<double>("solution") -> get(SymbolFocus::global);
	EXPECT_DOUBLE_EQ(result -> get(SymbolFocus::global), solution);
	int idx=1;
	
	while (SIM::getGlobalScope()->getAllSymbolNames<double>().count(string("result")+to_string(idx))) {
		auto result = SIM::findGlobalSymbol<double>(string("result")+to_string(idx));
		EXPECT_TRUE(result->flags().space_const);
		EXPECT_TRUE(result->flags().time_const);
		auto solution = SIM::findGlobalSymbol<double>("solution"+to_string(idx)) -> get(SymbolFocus::global);
		EXPECT_DOUBLE_EQ(result -> get(SymbolFocus::global), solution);
		idx++;
	}
}

TEST (ConstInit, UnorderedFunction) {
	auto file2 = ImportFile("const_function_unordered.xml");
	auto model = TestModel(file2.getDataAsString());

	model.run();
	auto result = SIM::findGlobalSymbol<double>("result");
	EXPECT_TRUE(result->flags().space_const);
	EXPECT_TRUE(result->flags().time_const);
	auto solution = SIM::findGlobalSymbol<double>("solution") -> get(SymbolFocus::global);
	EXPECT_DOUBLE_EQ(result -> get(SymbolFocus::global), solution);
	int idx=1;
	
	while (SIM::getGlobalScope()->getAllSymbolNames<double>().count(string("result")+to_string(idx))) {
		auto result = SIM::findGlobalSymbol<double>(string("result")+to_string(idx));
		EXPECT_TRUE(result->flags().space_const);
		EXPECT_TRUE(result->flags().time_const);
		auto solution = SIM::findGlobalSymbol<double>("solution"+to_string(idx)) -> get(SymbolFocus::global);
		EXPECT_DOUBLE_EQ(result -> get(SymbolFocus::global), solution);
		idx++;
	}
	
}
