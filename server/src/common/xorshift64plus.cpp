#include "common/xorshift64plus.h"

#include <cstring>
#include <cmath>

// implementation based on https://github.com/chfoo/seedyrng/blob/master/src/seedyrng/Xorshift64Plus.hx

void Random::setSeed(int64_t seed)
{
	if (seed == 0)
		this->seed = 1;
	else
		this->seed = seed;
	
	_state0 = seed >> (uint32_t)32;
	_state1 = seed & (uint32_t)0xFFFFFFFF;
}

void Random::writeState(char* state)
{
	int size = 0;

	memcpy(state, &seed, sizeof(seed));
	size += sizeof(seed);

	memcpy(&state[size], &_state0, sizeof(_state0));
	size += sizeof(_state0);

	memcpy(&state[size], &_state1, sizeof(_state1));
}

int32_t Random::nextInt()
{
	int32_t x = _state0;
	int32_t y = _state1;

	_state0 = y;
	x ^= x << PARAMETER_A;
	_state1 = x ^ y ^ (x >> PARAMETER_B) ^ (y >> PARAMETER_C);

	return (_state1 + y) & 0xFFFFFFFF;
}

int32_t Random::nextFullInt()
{
	int32_t num1 = nextInt();
	uint32_t num2 = nextInt();
	num2 = num2 >> 16 | num2 << 16;
	return num1 ^ num2;
}

double Random::random() {
	int32_t upper = nextFullInt() & 0x1FFFFF;
	int32_t lower = nextFullInt();
	double num = upper * pow(2, 32) + lower;
	return num * pow(2, -53); 
}

double Random::uniform(double low, double high) {
	return low + random() * (high - low); 
}