#include "gtest/gtest.h"
#include "3rdparty/muParser/muParser.h"
#include <memory>
using std::make_shared;

double return_nargs(const double* cargs, int nargs) {
	return nargs;
}

double return_4(double i) {
	return 4;
}

class return_4_generic : public mu::fun_class_generic {
	int argc() const override { return 1; };
	mu::value_type operator()(const double* args, void * data) const override {
		return 4;
	}
};

class return_data_generic : public mu::fun_class_generic {
	int argc() const override { return 0; };
	mu::value_type operator()(const double* args, void * data) const override {
		return *((double*)data);
	}
};

TEST (MUPARSER, OVERLOADING_MULTI) {
	auto parser = make_shared<mu::Parser>();
	parser->DefineFun("args",return_nargs);
	parser->DefineFun("args",return_4);
	
	// specific function overload superseeds multi
	parser->SetExpr("args(1)");
	EXPECT_EQ(parser->Eval(),4);
	// multi function overload solves all non-specific cases
	parser->SetExpr("args(1,1)");
	EXPECT_EQ(parser->Eval(),2);
}

TEST (MUPARSER, OVERLOADING_GENERIC) {
	auto parser = make_shared<mu::Parser>();
	parser->DefineFun("args",return_nargs);
	auto r4 = make_shared<return_4_generic>();
	parser->DefineFun("args",r4.get());
	auto rdata = make_shared<return_data_generic>();
	parser->DefineFun("data",rdata.get());
	// specific function overload superseeds multi
	parser->SetExpr("args(1)");
	EXPECT_EQ(parser->Eval(),4);
	// mult function overload solves all non-specific cases
	parser->SetExpr("args(1,1)");
	EXPECT_EQ(parser->Eval(),2);
	// multi function overload propagates anonymous data (used for SymbolFocus propagation)
	double data = 256;
	parser->SetExpr("data()");
	EXPECT_EQ(parser->Eval(&data),data);
	
}
