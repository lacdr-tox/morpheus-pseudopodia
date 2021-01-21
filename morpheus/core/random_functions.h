#ifndef RANDOM_FUNCTIONS_H
#define RANDOM_FUNCTIONS_H

#include "config.h"

// global random methods using a unique source of randomness to gain reproducability
bool getRandomBool();
double getRandom01();
double getRandomGauss(double s);
double getRandomGamma(double shape, double scale);
/// Produce a random number in the range [0,max_val]
uint getRandomUint(uint max_val);

void setRandomSeed(uint seed);

#endif
