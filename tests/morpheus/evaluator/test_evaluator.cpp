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
	try {
	auto evaluator = make_unique<ExpressionEvaluator<VDOUBLE>>("b*a",cache);
	evaluator->init();
	EXPECT_EQ(evaluator->get(SymbolFocus::global),VDOUBLE(2,4,4));
	auto evaluator2 = make_unique<ExpressionEvaluator<double>>("sqrt(a.x^2+a.y^2+a.z^2)",cache);
	EXPECT_EQ(evaluator2->safe_get(SymbolFocus::global),3);
	}
	catch (string e) {
		cout << e << endl;
	}
	
}


TEST (ExpressionEvaluator, ForeignScopeNamespaces) {
	auto scope = make_unique<Scope>();
	auto sub_scope = scope->createSubScope("sub");
	auto cache = make_shared<EvaluatorCache>(scope.get());
	auto sub_id = cache->addNameSpaceScope("sub", sub_scope);
	
	auto variable_a = make_shared<PrimitiveVariableSymbol<double>>("a","a", 1.0);
	scope->registerSymbol(variable_a);
	auto variable_sub_a = make_shared<PrimitiveVariableSymbol<double>>("a","a", 2.0);
	auto variable_sub_b = make_shared<PrimitiveVariableSymbol<double>>("b","b", 2.0);
	sub_scope->registerSymbol(variable_sub_a);
	sub_scope->registerSymbol(variable_sub_b);
	
	auto evaluator = make_unique<ExpressionEvaluator<double>>("a+sub.a",cache);
	evaluator->init();
	cache->setNameSpaceFocus(sub_id, SymbolFocus::global);
	EXPECT_EQ(evaluator->get(SymbolFocus::global),3);
	
	evaluator = make_unique<ExpressionEvaluator<double>>("a+sub.b",cache);
	evaluator->init();
	cache->setNameSpaceFocus(sub_id, SymbolFocus::global);
	EXPECT_EQ(evaluator->get(SymbolFocus::global),3);
	
	auto v_evaluator = make_unique<ExpressionEvaluator<VDOUBLE>>("sub.v.y * sub.v", scope.get());
	sub_id = v_evaluator->addNameSpaceScope("sub", sub_scope);
	auto variable_v = make_shared<PrimitiveVariableSymbol<VDOUBLE>>("v","v",VDOUBLE(1,2,2));
	sub_scope->registerSymbol(variable_v);
	v_evaluator->init();
	v_evaluator->setNameSpaceFocus(sub_id, SymbolFocus::global);
	EXPECT_EQ(v_evaluator->get(SymbolFocus::global),VDOUBLE(2,4,4));
}
