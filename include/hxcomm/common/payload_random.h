#pragma once
#include "hxcomm/common/payload.h"
#include <random>

namespace hxcomm::random {

/**
 * To-type-proxy to partially specialize function on a non-default-constructible argument type.
 * @tparam T Type to proxy for
 */
template <typename T>
struct type
{};

/**
 * Generate a payload type with random value(s).
 * @tparam PayloadType Type of payload
 * @param proxy Type proxy to payload type
 * @param gen Random number generator
 * @return Random payload
 */
template <typename PayloadType>
PayloadType random_payload(type<PayloadType> proxy, std::mt19937 gen) = delete;

/**
 * Generate a Bitset payload type with random value.
 * @tparam Tag Tag of instruction for type-safety
 * @tparam N Number of bits of Bitset paylaod
 * @param gen Random number generator
 * @return Random payload
 */
template <typename Tag, size_t N>
hxcomm::instruction::detail::payload::Bitset<Tag, N> random_payload(
    type<hxcomm::instruction::detail::payload::Bitset<Tag, N>>, std::mt19937 gen)
{
	std::bernoulli_distribution distribution;
	hate::bitset<N> value;
	for (size_t i = 0; i < N; ++i) {
		value.set(i, distribution(gen));
	}
	return hxcomm::instruction::detail::payload::Bitset<Tag, N>(value);
}

/**
 * Generate a Number payload type with random value.
 * @tparam Tag Tag of instruction for type-safety
 * @tparam T Number base type
 * @param gen Random number generator
 * @return Random payload
 */
template <typename Tag, typename T>
hxcomm::instruction::detail::payload::Number<Tag, T> random_payload(
    type<hxcomm::instruction::detail::payload::Number<Tag, T>>, std::mt19937 gen)
{
	std::uniform_int_distribution<T> random;
	return hxcomm::instruction::detail::payload::Number<Tag, T>(random(gen));
}

/**
 * Generate a payload type with random value(s).
 * @tparam PayloadType Type of payload
 * @param gen Random number generator
 * @return Random payload
 */
template <typename PayloadType>
PayloadType random_payload(std::mt19937 gen = std::mt19937(std::random_device{}()))
{
	return random_payload(type<PayloadType>{}, gen);
}

/**
 * Generate a payload type with random value(s) unequal to provided default value(s).
 * @tparam PayloadType Type of payload
 * @param default_payload Default payload value to exclude from random draw
 * @param gen Random number generator
 * @return Random payload
 */
template <typename PayloadType>
PayloadType random_payload_non_default(
    PayloadType const default_payload = PayloadType(),
    std::mt19937 gen = std::mt19937(std::random_device{}()))
{
	PayloadType ret = random_payload<PayloadType>(gen);
	while (ret == default_payload) {
		ret = random_payload<PayloadType>(gen);
	}
	return ret;
}

} // namespace hxcomm::random
