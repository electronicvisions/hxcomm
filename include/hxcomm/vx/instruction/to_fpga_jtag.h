#pragma once
#include <climits>
#include <boost/type_index.hpp>

#include "hate/math.h"
#include "hate/type_list.h"
#include "hxcomm/common/payload.h"
#include "rant/int.h"

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
	typedef hxcomm::instruction::detail::payload::Bitset<Ins, size> Payload;

	constexpr static Payload EXTEST{0};
	constexpr static Payload IDCODE{1};
	constexpr static Payload SAMPLE_PRELOAD{2};
	constexpr static Payload PLL_TARGET_REG{3};
	constexpr static Payload SHIFT_PLL{4};
	constexpr static Payload OMNIBUS_ADDRESS{5};
	constexpr static Payload OMNIBUS_DATA{6};
	constexpr static Payload OMNIBUS_REQUEST{7};
	constexpr static Payload BYPASS{127};
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
		typedef rant::integral_range<uint_fast8_t, max_num_bits_payload, min_num_bits_payload>
		    NumBits;

		Payload(
		    bool const keep_response = false,
		    NumBits const num_bits = NumBits(max_num_bits_payload),
		    hate::bitset<max_num_bits_payload> const payload = 0u) :
		    m_keep_response(keep_response), m_num_bits(num_bits), m_payload(payload)
		{}


		bool get_keep_response() const { return m_keep_response; }
		void set_keep_response(bool const value) { m_keep_response = value; }

		NumBits get_num_bits() const { return m_num_bits; }
		void set_num_bits(NumBits const value) { m_num_bits = value; }

		hate::bitset<max_num_bits_payload> get_payload() const { return m_payload; }
		void set_payload(hate::bitset<max_num_bits_payload> const& value) { m_payload = value; }

		bool operator==(Payload const& other) const
		{
			return (
			    (m_keep_response == other.m_keep_response) && (m_num_bits == other.m_num_bits) &&
			    (m_payload == other.m_payload));
		}

		bool operator!=(Payload const& other) const { return !(*this == other); }

		template <class SubwordType = unsigned long>
		hate::bitset<size, SubwordType> encode() const
		{
			return (value_type(m_keep_response) << (size - padded_num_bits_keep_response)) |
			       (value_type(m_num_bits) << padded_num_bits_payload) | m_payload;
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
