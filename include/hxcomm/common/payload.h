#pragma once
#include <climits>
#include <stddef.h>

#include "halco/common/geometry.h"
#include "hate/bitset.h"
#include "hate/type_list.h"

/**
 * Payload formatting.
 * Each payload type defines a value_type used for encoding the payload to a bitstream.
 * The encode member function encodes payload data to a bitstream.
 * The decode member fucntion decodes a bitstream to payload data.
 */
namespace hxcomm::instruction::detail::payload {

/**
 * Payload for bitset data.
 * @tparam Tag Type of tag class
 * @tparam N Number of bits of payload
 */
template <typename Tag, size_t N>
class Bitset : public hate::bitset<N>
{
public:
	typedef hate::bitset<N> value_type;

	template <typename... Args>
	constexpr Bitset(Args... args) : hate::bitset<N>(args...)
	{}

	template <typename SubwordType = unsigned long>
	constexpr hate::bitset<N, SubwordType> encode() const
	{
		return *this;
	}

	template <typename SubwordType = unsigned long>
	void decode(hate::bitset<N, SubwordType> const& data)
	{
		*this = data;
	}
};

/**
 * Payload for number-type data.
 * @tparam Tag Type of tag class
 * @tparam N Number of bits of payload
 * @tparam T underlying number type
 */
template <typename Tag, typename T>
class Number : public halco::common::detail::BaseType<Number<Tag, T>, T>
{
public:
	typedef hate::bitset<sizeof(T) * CHAR_BIT> value_type;
	typedef halco::common::detail::BaseType<Number<Tag, T>, T> base_t;

	constexpr explicit Number(T const value = 0) : base_t(value) {}

	template <typename SubwordType = unsigned long>
	constexpr hate::bitset<value_type::size, SubwordType> encode() const
	{
		return this->value();
	}

	template <typename SubwordType = unsigned long>
	void decode(hate::bitset<value_type::size, SubwordType> const& data)
	{
		*static_cast<base_t*>(this) = static_cast<T>(data);
	}

private:
	static_assert(std::is_integral<T>::value, "number is supposed to be used with integer type.");
	static_assert(
	    value_type::size <= value_type::num_bits_per_word,
	    "number only works with <= word width number type.");
};

/**
 * Payload for ranged-type data.
 * @tparam Tag Type of tag class
 * @tparam N Number of bits of payload
 * @tparam T underlying type of ranged type
 * @tparam Min Minimal value in range
 * @tparam Max Maximal value in range
 */
template <typename Tag, size_t N, typename T, uintmax_t Min, uintmax_t Max>
class Ranged : public halco::common::detail::RantWrapper<Ranged<Tag, N, T, Min, Max>, T, Max, Min>
{
public:
	typedef hate::bitset<N> value_type;
	typedef halco::common::detail::RantWrapper<Ranged<Tag, N, T, Min, Max>, T, Max, Min> rant_t;

	constexpr explicit Ranged(uintmax_t const value = 0) : rant_t(value) {}

	template <typename SubwordType = unsigned long>
	constexpr hate::bitset<N, SubwordType> encode() const
	{
		return this->value();
	}
	template <typename SubwordType = unsigned long>
	void decode(hate::bitset<N, SubwordType> const& data)
	{
		*static_cast<rant_t*>(this) = static_cast<T>(data);
	}
};

} // namespace hxcomm::instruction::detail::payload
