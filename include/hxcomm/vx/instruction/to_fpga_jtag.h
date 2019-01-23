#pragma once
#include <climits>

#include "hate/math.h"
#include "hxcomm/common/payload.h"

/** JTAG instructions to the fpga. */
namespace hxcomm::vx::instruction::to_fpga_jtag {

/** Reset state machine. */
struct init
{
	constexpr static size_t size = 0;
	typedef hxcomm::instruction::detail::payload::bitset<init, size> payload_type;
};

/** Set slow-down scaling factor of clock of the JTAG communication. */
struct scaler
{
	typedef uint8_t value_type;
	constexpr static size_t size = sizeof(value_type) * CHAR_BIT;
	typedef hxcomm::instruction::detail::payload::number<scaler, value_type> payload_type;
};

/** Select instruction register on the Hicann. */
struct ins
{
	constexpr static size_t size = 7;
	typedef hxcomm::instruction::detail::payload::rant<ins, size, uint8_t, 0, 127> payload_type;

	static const payload_type EXTEST;
	static const payload_type IDCODE;
	static const payload_type SAMPLE_PRELOAD;
	static const payload_type PLL_TARGET_REG;
	static const payload_type SHIFT_PLL;
	static const payload_type OMNIBUS_ADDRESS;
	static const payload_type OMNIBUS_DATA;
	static const payload_type OMNIBUS_REQUEST;
	static const payload_type BYPASS;
};

/** Data instruction. */
struct data
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
	class payload_type
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

		payload_type();
		payload_type(
		    bool const keep_response,
		    NumBits const num_bits,
		    hate::bitset<max_num_bits_payload> const payload);

		bool get_keep_response() const;
		void set_keep_response(bool value);

		NumBits get_num_bits() const;
		void set_num_bits(NumBits value);

		hate::bitset<max_num_bits_payload> get_payload() const;
		void set_payload(hate::bitset<max_num_bits_payload> const& value);

		bool operator==(payload_type const& other) const;
		bool operator!=(payload_type const& other) const;

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

	private:
		/** Select whether to keep the JTAG_from_hicann::data response. */
		bool m_keep_response;
		NumBits m_num_bits;
		/** Data to be inserted into the previously selected instruction register. */
		hate::bitset<max_num_bits_payload> m_payload;
	};
};

/** Dictionary of all to_fpga_jtag instructions. */
typedef hate::type_list<init, scaler, ins, data> dictionary;

} // namespace hxcomm::vx::instruction::to_fpga_jtag
