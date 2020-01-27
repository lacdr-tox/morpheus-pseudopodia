#include "gtest/gtest.h"
#include "model_test.h"
#include "core/simulation.h"
#include "core/data_mapper.h"

#include <boost/math/distributions/normal.hpp>
#include <boost/math/distributions/students_t.hpp>
#include "Eigen/Eigen"
using namespace boost::math;
// no valarrays here. these just suck !!
typedef  Eigen::ArrayXd Sample;

class Gaussian {
	double mean;
	double stddev;
public:
	Gaussian(double m, double v) : mean(m), stddev(v) {};
	bool testSameMean(double alpha, const Sample& data ) const { 
		auto avg = data.sum()/data.size();
		
		auto t_stat = (avg-mean) * sqrt(double(data.size())) / stddev;
		students_t dist(stddev);
		double quantil = cdf( complement(dist, fabs(t_stat)));
		
		return quantil > alpha/2;
	}
	
	double m() const { return mean; }
	double s() const { return stddev; }
};

::testing::AssertionResult EQ_STAT(const char* m_expr, const char* n_expr, const Sample& m, const Gaussian& n) {
	if (n.testSameMean(0.05, m))
		return ::testing::AssertionSuccess();
	else
		return ::testing::AssertionFailure() << "Error in statistical assertion:  " << m_expr << " is not whithin 95% quantil of Gaussion (" << n.m() << ", " << n.s() << ")" ;
}

TEST (DirectedMotion, VELOCITY) {
	ImportFile("dir_motion.xml");
	int replicates = 10;
	
	vector<VDOUBLE> velocities;
	auto model = TestModel(GetFileString("dir_motion.xml"));
	model.setParam("moving_dir", "0.01,0,0");
	
	for (auto i=0; i<replicates; i++) {
		model.run();
		velocities.push_back( 
			SIM::findGlobalSymbol<VDOUBLE>("velocity") -> get(SymbolFocus::global)
		);
	}
	const double axial_msd = SIM::findGlobalSymbol<double>("expected_msd_mean") -> get(SymbolFocus::global);
	const double axial_msd_stddev = SIM::findGlobalSymbol<double>("expected_msd_stddev") -> get(SymbolFocus::global);
	
	// Map absolut velocity distribution
	Sample vel_abs(velocities.size());
	std::transform(velocities.begin(), velocities.end(), &vel_abs[0], [](const VDOUBLE& val) -> double { return val.abs(); });
	// Map velocity orientation distribution
	EXPECT_PRED_FORMAT2( EQ_STAT,  vel_abs, Gaussian(axial_msd, axial_msd_stddev) );
}
