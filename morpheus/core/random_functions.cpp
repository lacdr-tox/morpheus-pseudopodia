#include "random_functions.h"

// make a unique source of randomness available to everyone
vector<mt19937> random_engines;

typedef std::normal_distribution<double> RNG_GaussDist;
typedef std::gamma_distribution<double> RNG_GammaDist;

bool getRandomBool() {
	return random_engines[ omp_get_thread_num() ]()<random_engines[ omp_get_thread_num() ].max()/2;
}

double getRandom01() {
	static uniform_real_distribution <double> rnd(0.0,1.0);
	return rnd(random_engines[omp_get_thread_num()]);
}

// random gaussian distribution of stddev s
double getRandomGauss(double s) {
	RNG_GaussDist rnd( 0.0, s);
	return rnd(random_engines[omp_get_thread_num()]);
}

double getRandomGamma(double shape, double scale) {

    RNG_GammaDist rnd( shape );
    return scale*rnd(random_engines[omp_get_thread_num()]);

}

uint getRandomUint(uint max_val) {
	uniform_int_distribution<uint> rnd(0,max_val);
    return rnd(random_engines[omp_get_thread_num()]);
}

void setRandomSeed(uint random_seed)
{
		// initialize multiple random engines (one for each thread) and set seed
	// 1. make vector of random engines
	uint numthreads = 1;		
#pragma omp parallel
	{
		numthreads = omp_get_num_threads();
	}
	random_engines.resize( numthreads );

	// 2. set random seed of first engine taken from XML
	random_engines[0].seed(random_seed);
	cout << "Random seed of master thread = " << random_seed << endl;

	// 3. generate random seeds for other engines using the first engine.
	vector<uint> random_seeds(numthreads,0);
	for(uint i=1; i<numthreads; i++){
		uniform_int_distribution<> rnd(0,9999999);
		random_seeds[i] = rnd(random_engines[0]);
	}

	// 4. set random seed of other engines (for other threads)
#pragma omp parallel
	{
		uint thread = omp_get_thread_num();
		if( thread > 0){
			random_engines[ thread ].seed(random_seeds[ thread ] );
#pragma omp critical
			cout << "Random seed of thread " << thread << " = " << random_seeds[ thread ] << endl;
		}
	}
}

