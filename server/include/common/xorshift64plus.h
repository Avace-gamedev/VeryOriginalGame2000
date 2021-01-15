#pragma once

#include <stdint.h>

#define XORSHIFT64PLUS_STATE_SIZE 16

class Random
{
	const int PARAMETER_A = 17;
	const int PARAMETER_B = 7;
	const int PARAMETER_C = 16;

	int64_t seed;
	uint32_t _state0;
	uint32_t _state1;

	int32_t nextInt();
	int32_t nextFullInt();


public:
	Random() { setSeed(1); };

	void setSeed(int64_t seed);
	
	void writeState(char* state);

	double random();
	double uniform(double low, double high);
};