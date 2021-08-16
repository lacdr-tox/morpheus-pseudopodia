#include "gtest/gtest.h"
#include "core/system.h"
#include "core/property.h"
#include "core/time_scheduler.h"

using std::string;
// using std::unique_ptr;
const string ode_1 = {"<DiffEqn symbol-ref=\"x\"><Expression>lambda * x</Expression></DiffEqn>"};
double ode_1_x_init = 1;
double ode_1_lambda = 1.3;
double ode_1_time = 0.5;
double ode_1_time_scaling = 10;
// double ode_1_res_x = 665.141633044362
double ode_1_res_x = ode_1_x_init * exp(ode_1_lambda*ode_1_time*ode_1_time_scaling);
const string ode_sys_1 = string("<System time-scaling=\"") + to_str(ode_1_time_scaling) + "\">" + ode_1 + "</System>";
auto x_ode_sys_1 =  XMLNode::parseString(ode_sys_1.c_str());


/** Test precision of data serialization **/
const double min_prec_tol = 10e-25;
const double rel_prec = 10e-6;

::testing::AssertionResult EQ_PREC(const char* m_expr, const char* n_expr, double m, double n) {
	double d = m-n; auto d_err = rel_prec*abs(n) + min_prec_tol;
	if (d < d_err  && d > -d_err)
		return ::testing::AssertionSuccess();
	else
		return ::testing::AssertionFailure() << "Error in Serialization " << m_expr << "\n" << "Relative error of '" << m << "' and '" << n << "' is above " << rel_prec;
}

TEST (System, Solver_Heun) {
// 	SIM::wipe();
	auto scope = make_shared<Scope>();
	auto time = make_shared<PrimitiveVariableSymbol<double>>(SymbolBase::Time_symbol,"time",0);
	auto var_x = make_shared<PrimitiveVariableSymbol<double>>("x","x",ode_1_x_init);
	auto const_lambda = make_shared<PrimitiveConstantSymbol<double>>("lambda","lambda",ode_1_lambda);
	scope->registerSymbol(time);
	scope->registerSymbol(var_x);
	scope->registerSymbol(const_lambda);
	
	auto x_sys = x_ode_sys_1.deepCopy();
	x_sys.addAttribute("solver","heun");
// 	std::cout <<  x_sys.createXMLString() << endl;
	auto sys = make_unique<System>(System::CONTINUOUS);
	sys->loadFromXML(x_sys, scope.get());
	sys->setTimeStep(ode_1_time/1000);
	sys->init();
	
	uint i=0;
	ASSERT_GT(sys->timeStep(),0);
	while (time->get() < ode_1_time) {
		sys->compute(SymbolFocus::global);
		time->set(time->get() + sys->timeStep());
		i++;
	}
// 	std::cout << "Solving System took " << i << " substeps" << std::endl;
	EXPECT_LE(abs(ode_1_res_x - var_x->get(SymbolFocus::global)), ode_1_res_x * 1e-3);
// 	EXPECT_PRED_FORMAT2(EQ_PREC,ode_1_res_x,var_x->get());
}

TEST (System, Solver_RK4) {
	SIM::wipe();
	auto scope = make_shared<Scope>();
	auto time = make_shared<PrimitiveVariableSymbol<double>>(SymbolBase::Time_symbol,"time",0);
	auto var_x = make_shared<PrimitiveVariableSymbol<double>>("x","x",ode_1_x_init);
	auto const_lambda = make_shared<PrimitiveConstantSymbol<double>>("lambda","lambda",ode_1_lambda);
	scope->registerSymbol(var_x);
	scope->registerSymbol(const_lambda);
	auto x_sys = x_ode_sys_1.deepCopy();
	x_sys.addAttribute("solver","fixed4");
	auto sys = make_unique<System>(System::CONTINUOUS);
// 	std::cout <<  x_sys.createXMLString() << endl;
	sys->loadFromXML(x_sys, scope.get());
	double time_step = ode_1_time/100;
	sys->init();
	sys->setTimeStep(time_step);
	
	uint i=0;
	ASSERT_GT(sys->timeStep(),0);
	while (time->get() < ode_1_time) {
		sys->compute(SymbolFocus::global);
		time->set(time->get() + sys->timeStep());
		i++;
	}
	EXPECT_PRED_FORMAT2(EQ_PREC,ode_1_res_x,var_x->get(SymbolFocus::global));
// 	std::cout << "Solving System took " << i << " substeps" << std::endl;
}

TEST (System, Solver_RK45) {
	SIM::wipe();
	auto scope = make_shared<Scope>();
	auto time = make_shared<PrimitiveVariableSymbol<double>>(SymbolBase::Time_symbol,"time",0);
	auto var_x = make_shared<PrimitiveVariableSymbol<double>>("x","x",ode_1_x_init);
	auto const_lambda = make_shared<PrimitiveConstantSymbol<double>>("lambda","lambda",ode_1_lambda);
	scope->registerSymbol(var_x);
	scope->registerSymbol(const_lambda);
	auto x_sys = x_ode_sys_1.deepCopy();
	x_sys.addAttribute("solver","adaptive45");
	x_sys.addAttribute("solver-eps", to_cstr(rel_prec/10));
// 	std::cout <<  x_sys.createXMLString() << endl;
	auto sys = make_unique<System>(System::CONTINUOUS);
	sys->loadFromXML(x_sys, scope.get());
	sys->setTimeStep(ode_1_time);
	sys->init();
	
	uint i=0;
	ASSERT_GT(sys->timeStep(),0);
	while (time->get() < ode_1_time) {
		sys->compute(SymbolFocus::global);
		time->set(time->get() + sys->timeStep());
		i++;
	}
	EXPECT_PRED_FORMAT2(EQ_PREC,ode_1_res_x,var_x->get(SymbolFocus::global));
	std::cout << "Solving System took " << i << " substeps" << std::endl;
}

TEST (System, Solver_RK23) {
	SIM::wipe();
	auto scope = make_shared<Scope>();
	auto time = make_shared<PrimitiveVariableSymbol<double>>(SymbolBase::Time_symbol,"time",0);
	auto var_x = make_shared<PrimitiveVariableSymbol<double>>("x","x",ode_1_x_init);
	auto const_lambda = make_shared<PrimitiveConstantSymbol<double>>("lambda","lambda",ode_1_lambda);
	scope->registerSymbol(var_x);
	scope->registerSymbol(const_lambda);
	auto x_sys = x_ode_sys_1.deepCopy();
	x_sys.addAttribute("solver","adaptive23");
	x_sys.addAttribute("solver-eps", to_cstr(rel_prec/10));
// 	std::cout <<  x_sys.createXMLString() << endl;
	auto sys = make_unique<System>(System::CONTINUOUS);
	sys->loadFromXML(x_sys, scope.get());
	sys->setTimeStep(ode_1_time);
	sys->init();
	
	uint i=0;
	ASSERT_GT(sys->timeStep(),0);
	while (time->get() < ode_1_time) {
		sys->compute(SymbolFocus::global);
		time->set(time->get() + sys->timeStep());
		i++;
	}
	EXPECT_PRED_FORMAT2(EQ_PREC,ode_1_res_x,var_x->get(SymbolFocus::global));
// 	std::cout << "Solving System took " << i << " substeps" << std::endl;
}



/*
TEST (System, Solver_RK4) {
	SIM::wipe();
	auto scope = SIM::getGlobalScope();
	auto time = make_shared<PrimitiveVariableSymbol<double>>(SymbolBase::Time_symbol,"time",0);
	auto var_x = make_shared<PrimitiveVariableSymbol<double>>("x","x",ode_1_x_init);
	auto const_lambda = make_shared<PrimitiveConstantSymbol<double>>("lambda","lambda",ode_1_lambda);
	scope->registerSymbol(var_x);
	scope->registerSymbol(const_lambda);
	auto x_sys = x_ode_sys_1.deepCopy();
	x_sys.addAttribute("solver","runge-kutta");
	x_sys.addAttribute("time-step", to_cstr(ode_1_time/200));
	auto sys = make_unique<ContinuousSystem>();
	std::cout <<  x_sys.createXMLString() << endl;
	sys->loadFromXML(x_sys, scope);
	sys->init(scope);
	
	TimeScheduler::setStopTime(ode_1_time);
	TimeScheduler::init(scope);
	TimeScheduler::compute();
	EXPECT_PRED_FORMAT2(EQ_PREC,ode_1_res_x,var_x->get(SymbolFocus::global));
}*/
