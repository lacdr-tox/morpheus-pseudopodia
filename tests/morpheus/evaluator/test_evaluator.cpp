#include "gtest/gtest.h"
#include <memory>
#include "core/expression_evaluator.h"

TEST (Scope, UndefinedSymbol) {
	auto scope = make_unique<Scope>();
	auto variable_a = make_shared<PrimitiveVariableSymbol<double>>("a","a test a",3.0);
	scope->registerSymbol(variable_a);
	
	bool symbol_undefined = false;
	try {
		auto k = scope->findSymbol<double>("b");
	}
	catch (SymbolError& e) {
		if (e.type() == SymbolError::Type::Undefined) {
			symbol_undefined = true;
		}
	}
	EXPECT_TRUE(symbol_undefined);
	
	symbol_undefined = false;
	try {
		auto k = scope->findSymbol<double>("b",true);
	}
	catch (SymbolError& e) {
		if (e.type() == SymbolError::Type::Undefined) {
			symbol_undefined = true;
		}
	}
	EXPECT_TRUE(symbol_undefined);
}

TEST (ExpressionEvaluator, Intialisation) {
	auto scope = make_unique<Scope>();
	auto cache = make_shared<EvaluatorCache>(scope.get());
	auto variable_a = make_shared<PrimitiveVariableSymbol<double>>("a","a test a",3.0);
	scope->registerSymbol(variable_a);
	
	// safe_get() auto-initializes evaluator with the scope provided by the cache.
	auto evaluator = make_unique<ExpressionEvaluator<double>>("a+1",cache);
	EXPECT_EQ(evaluator->safe_get(SymbolFocus::global),4);
	
}

TEST (ExpressionEvaluator, JoinedCache) {
	auto scope = make_unique<Scope>();
	auto cache = make_shared<EvaluatorCache>(scope.get());
	auto variable_a = make_shared<PrimitiveVariableSymbol<double>>("a","a test a",3.0);
	auto variable_b = make_shared<PrimitiveVariableSymbol<double>>("b","a test b",12.0);
	scope->registerSymbol(variable_a);
	scope->registerSymbol(variable_b);
	
	auto evaluator = make_unique<ExpressionEvaluator<double>>("a+1",cache);
	auto evaluator2 = make_unique<ExpressionEvaluator<double>>("a+1+b",cache);
	evaluator->init();
	evaluator2->init();

	EXPECT_EQ(evaluator->get(SymbolFocus::global),4);
	EXPECT_EQ(evaluator2->get(SymbolFocus::global),16);
	
	EXPECT_EQ(cache->getExternalSymbols().size(), 2);
}


TEST (ExpressionEvaluator, VectorSymbols) {
	auto scope = make_unique<Scope>();
	auto cache = make_shared<EvaluatorCache>(scope.get());
	auto variable_a = make_shared<PrimitiveVariableSymbol<VDOUBLE>>("a","a test a",VDOUBLE(1,2,2));
	scope->registerSymbol(variable_a);
	auto variable_b = make_shared<PrimitiveVariableSymbol<double>>("b","a test b",2);
	scope->registerSymbol(variable_b);
	
	auto evaluator = make_unique<ExpressionEvaluator<VDOUBLE>>("b*a",cache);
	evaluator->init();
	EXPECT_EQ(evaluator->get(SymbolFocus::global),VDOUBLE(2,4,4));
	auto evaluator2 = make_unique<ExpressionEvaluator<double>>("sqrt(a.x^2+a.y^2+a.z^2)",cache);
	EXPECT_EQ(evaluator2->safe_get(SymbolFocus::global),3);
	
}
