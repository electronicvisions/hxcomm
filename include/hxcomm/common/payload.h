#pragma once
#include "hate/bitset.h"
#include "hate/type_index.h"
#include <climits>
#include <stddef.h>

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
	/** Underlying value type. */
	typedef hate::bitset<N> value_type;

	/**
	 * Construct a bitset payload.
	 * @tparam Args Argument types to construct from
	 * @param args Agruments to construct from
	 */
	template <typename... Args>
	constexpr explicit Bitset(Args... args) : value_type(args...)
	{}

	/**
	 * Encode into a bitset of size N (no-op for Bitset).
	 * @tparam SubwordType Word type used in bitset representation
	 * @return Payload encoded as bitset
	 */
	template <typename SubwordType = unsigned long>
	constexpr hate::bitset<N, SubwordType> encode() const
	{
		return *this;
	}

	/**
	 * Decode from a bitset of size N (no-op for Bitset).
	 * @tparam SubwordType Word type used in bitset representation
	 * @param data Bitset data to decode
	 */
	template <typename SubwordType = unsigned long>
	void decode(hate::bitset<N, SubwordType> const& data)
	{
		*this = Bitset(data);
	}

	friend std::ostream& operator<<(std::ostream& os, Bitset const& value)
	{
		os << hate::full_name<Tag>() << "(" << static_cast<value_type>(value) << ")";
		return os;
	}
};


/**
 * Payload for number-type data.
 * @tparam Tag Type of tag class
 * @tparam N Number of bits of payload
 * @tparam T underlying number type
 */
template <typename Tag, typename T>
class Number
{
public:
	constexpr static size_t size = sizeof(T) * CHAR_BIT;

	/**
	 * Construct a Number payload.
	 * @param value Value to construct from
	 */
	constexpr explicit Number(T const value = 0) : m_value(value) {}

	/**
	 * Encode into a bitset of size N.
	 * @tparam SubwordType Word type used in bitset representation
	 * @return Payload encoded as bitset
	 */
	template <typename SubwordType = unsigned long>
	constexpr hate::bitset<size, SubwordType> encode() const
	{
		return m_value;
	}

	/**
	 * Decode from a bitset of size N.
	 * @tparam SubwordType Word type used in bitset representation
	 * @param data Bitset data to decode
	 */
	template <typename SubwordType = unsigned long>
	void decode(hate::bitset<size, SubwordType> const& data)
	{
		m_value = static_cast<T>(data);
	}

	friend std::ostream& operator<<(std::ostream& os, Number const& value)
	{
		os << hate::full_name<Tag>() << "("
		   << static_cast<std::conditional_t<std::is_same<unsigned char, T>::value, uintmax_t, T>>(
		          value.m_value)
		   << ")";
		return os;
	}

	T value() const { return m_value; }
	operator T() const { return m_value; }

	bool operator==(Number const& other) const { return m_value == other.m_value; }
	bool operator!=(Number const& other) const { return !(*this == other); }

private:
	static_assert(std::is_integral<T>::value, "Number is supposed to be used with integer type.");
	T m_value;
};

} // namespace hxcomm::instruction::detail::payload
