#pragma once
#include <climits>
#include <boost/type_index.hpp>

#include "halco/common/geometry.h"
#include "hate/math.h"
#include "hate/type_list.h"
#include "hxcomm/common/payload.h"

/** JTAG instructions to the fpga. */
namespace hxcomm::vx::instruction::to_fpga_jtag {

/** Reset state machine. */
struct Init
{
	constexpr static size_t size = 0;
	typedef hxcomm::instruction::detail::payload::Bitset<Init, size> Payload;
};

/** Set slow-down scaling factor of clock of the JTAG communication. */
struct Scaler
{
	typedef uint8_t value_type;
	constexpr static size_t size = sizeof(value_type) * CHAR_BIT;
	typedef hxcomm::instruction::detail::payload::Number<Scaler, value_type> Payload;
};

/** Select instruction register on the Hicann. */
struct Ins
{
	constexpr static size_t size = 7;
	typedef hxcomm::instruction::detail::payload::Ranged<Ins, size, uint8_t, 0, 127> Payload;

	static const Payload EXTEST;
	static const Payload IDCODE;
	static const Payload SAMPLE_PRELOAD;
	static const Payload PLL_TARGET_REG;
	static const Payload SHIFT_PLL;
	static const Payload OMNIBUS_ADDRESS;
	static const Payload OMNIBUS_DATA;
	static const Payload OMNIBUS_REQUEST;
	static const Payload BYPASS;
};

/** Data instruction. */
struct Data
{
	constexpr static size_t max_num_bits_payload = 33;
	constexpr static size_t min_num_bits_payload = 3;

	constexpr static size_t padded_num_bits_payload =
	    hate::math::round_up_to_multiple(max_num_bits_payload, CHAR_BIT);
	constexpr static size_t padded_num_bits_num_bits_payload =
	    hate::math::round_up_to_multiple(hate::math::num_bits(max_num_bits_payload), CHAR_BIT);
	constexpr static size_t padded_num_bits_keep_response =
	    hate::math::round_up_to_multiple(1, CHAR_BIT);

	constexpr static size_t size =
	    padded_num_bits_payload + padded_num_bits_num_bits_payload + padded_num_bits_keep_response;

	/** Payload of a data instruction. */
	class Payload
	{
	public:
		typedef hate::bitset<size> value_type;

		/**
		 * Select the amount of bits of payload field to be shifted in instruction
		 * register previously selected by an ins instruction.
		 */
		struct NumBits
		    : public halco::common::detail::
		          RantWrapper<NumBits, uint8_t, max_num_bits_payload, min_num_bits_payload>
		{
			explicit NumBits(uintmax_t const value = max_num_bits_payload) : rant_t(value) {}
		};

		Payload();
		Payload(
		    bool const keep_response,
		    NumBits const num_bits,
		    hate::bitset<max_num_bits_payload> const payload);

		bool get_keep_response() const;
		void set_keep_response(bool value);

		NumBits get_num_bits() const;
		void set_num_bits(NumBits value);

		hate::bitset<max_num_bits_payload> get_payload() const;
		void set_payload(hate::bitset<max_num_bits_payload> const& value);

		bool operator==(Payload const& other) const;
		bool operator!=(Payload const& other) const;

		template <class SubwordType = unsigned long>
		hate::bitset<size, SubwordType> encode() const
		{
			return (value_type(m_keep_response) << (size - padded_num_bits_keep_response)) |
			       (value_type(m_num_bits.value()) << padded_num_bits_payload) | m_payload;
		}

		template <class SubwordType = unsigned long>
		void decode(hate::bitset<size, SubwordType> const& data)
		{
			m_keep_response = data.test(size - padded_num_bits_keep_response);
			m_num_bits = NumBits((data >> padded_num_bits_payload)
			                         .reset(padded_num_bits_keep_response)
			                         .to_uintmax());
			m_payload = data;
		}

		friend std::ostream& operator<<(std::ostream& os, Payload const& value)
		{
			os << boost::typeindex::type_id<Data>().pretty_name()
			   << "(keep_response: " << std::boolalpha << value.m_keep_response
			   << ", num_bits: " << static_cast<uintmax_t>(value.m_num_bits)
			   << ", payload: " << value.m_payload << ")";
			return os;
		}

	private:
		/** Select whether to keep the JTAG_from_hicann::data response. */
		bool m_keep_response;
		NumBits m_num_bits;
		/** Data to be inserted into the previously selected instruction register. */
		hate::bitset<max_num_bits_payload> m_payload;
	};
};

/** Dictionary of all to_fpga_jtag instructions. */
typedef hate::type_list<Init, Scaler, Ins, Data> Dictionary;

} // namespace hxcomm::vx::instruction::to_fpga_jtag
