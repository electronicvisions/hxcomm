#include "test-helper.h"

size_t random_integer(size_t const min, size_t const max)
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<size_t> random(min, max);
	return random(gen);
}
