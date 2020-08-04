#pragma once
#include "hate/bitset.h"
#include <random>
#include <gtest/gtest.h>

namespace hxcomm::test {

/**
 * Draw random integer value in specified bounds.
 * @param min Minimal value to be drawn
 * @param max Maximal value to be drawn
 */
size_t random_integer(size_t min, size_t max);

/**
 * Draw a random bitset of size N.
 * @tparam N number of bits of bitset
 * @return random bitset
 */
template <size_t N>
hate::bitset<N> random_bitset()
{
	static thread_local std::mt19937 g{std::random_device{}()};
	static thread_local std::bernoulli_distribution d;

	hate::bitset<N> ret;
	for (size_t i = 0; i < N; ++i) {
		ret.set(i, d(g));
	}
	return ret;
}

/**
 * Draw a type T variable out of all values but the specified default.
 * @tparam T type
 * @param default_value Default value to exclude from random draw
 * @return random value
 */
template <class T>
T draw_non_default_value(T default_value)
{
	static thread_local std::mt19937 g{std::random_device{}()};
	static thread_local std::uniform_int_distribution<uintmax_t> d;

	T rnd;
	do {
		rnd = T(d(g));
	} while (rnd == default_value);
	return rnd;
}

/**
 * Draw a ranged type T variable out of all values but the specified default.
 * @tparam T Ranged type
 * @param default_value Default value to exclude from random draw
 * @return random value from {T::min,...,T::max} \ default_value
 */
template <class T>
T draw_ranged_non_default_value(uintmax_t default_value)
{
	static thread_local std::mt19937 g{std::random_device{}()};
	static thread_local std::uniform_int_distribution<uintmax_t> d;

	uintmax_t rnd;
	do {
		rnd = (d(g) % (T::max + 1));
	} while ((rnd == default_value) || (rnd > T::max) || (rnd < T::min));
	return T(rnd);
}

} // namespace hxcomm::test
