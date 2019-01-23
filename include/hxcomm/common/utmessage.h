#pragma once
#include "hxcomm/common/cerealization.h"

#include "hate/bitset.h"
#include "hate/math.h"
#include "hate/type_list.h"
#include "hate/visibility.h"

namespace cereal {
class access;
} // namespace cereal

namespace hxcomm {

/**
 * Container for a UT message used for host-FPGA communication.
 * It is aligned to subword width. The header is left-aligned, the payload is right-aligned. The
 * leftmost header bit is reserved for a comma, which for valid UT messages is always set to false.
 * The header is calculated as the instruction's position in the dictionary.
 * @tparam HeaderAlignment Alignment of header in bits
 * @tparam SubwordType Type of subword which's width corresponds to the messages alignment
 * @tparam Dictionary Dictionary of instructions
 * @tparam Instruction Instruction to encode in UT message
 */
template <size_t HeaderAlignment, typename SubwordType, typename Dictionary, typename Instruction>
class ut_message
{
public:
	typedef Instruction instruction_type;
	typedef Dictionary dictionary_type;

	/** Number of bits of the comma. */
	static constexpr size_t comma_width = 1;
	/** Number of bits of a subword. */
	static constexpr size_t subword_width = sizeof(SubwordType) * CHAR_BIT;
	/** Number of bits of the header. The comma sits left-aligned in the header. */
	static constexpr size_t header_width = hate::math::round_up_to_multiple(
	    comma_width + hate::math::num_bits(hate::type_list_size<Dictionary>::value - 1),
	    HeaderAlignment);
	/** Number of subwords. */
	static constexpr size_t num_subwords = hate::math::round_up_integer_division(
	    header_width + Instruction::size, sizeof(SubwordType) * CHAR_BIT);

	/** Total number of bits of a ut_message. */
	static constexpr size_t word_width = subword_width * num_subwords;
	/** Number of bits of the payload field. */
	static constexpr size_t payload_width = Instruction::size;

	/** Word-type of a ut_message. */
	typedef hate::bitset<word_width, SubwordType> word_type;
	/** Type of the valid field. */
	typedef bool comma_type;
	/** Type of the header field. */
	typedef hate::bitset<header_width, SubwordType> header_type;
	/** Type of the payload field. */
	typedef hate::bitset<payload_width, SubwordType> payload_type;

	/** Default construct ut_message with zeroed payload. */
	constexpr ut_message();

	/**
	 * Construct ut_message with header and payload.
	 * @param payload Payload bitset to fill ut_message payload field with
	 */
	constexpr ut_message(payload_type const& payload);

	/**
	 * Construct ut_message with header and payload.
	 * @param payload Payload to fill ut_message payload field with
	 */
	constexpr ut_message(typename Instruction::payload_type const& payload);

	/**
	 * Get underlying bitset.
	 * @return Value of ut_message as word_type
	 */
	constexpr word_type get_raw() const;

	/**
	 * Get header.
	 * @return Bitset containing header
	 */
	constexpr static header_type get_header();

	/**
	 * Get payload.
	 * @return Bitset containing payload
	 */
	constexpr payload_type get_payload() const;

	/**
	 * Set payload.
	 * @param value Bitset containing payload
	 */
	constexpr void set_payload(payload_type const& value);

	/**
	 * Encode message from an instruction's payload type.
	 * @param value Payload to encode
	 */
	constexpr void encode(typename Instruction::payload_type const& value);

	/**
	 * Decode message to its instruction's payload type.
	 * @return Payload of message
	 */
	constexpr typename Instruction::payload_type decode() const;

	/**
	 * Equality operator.
	 * @param other ut_message to compare to
	 * @return Boolean result of comparison
	 */
	bool operator==(ut_message const& other) const;

	/**
	 * Inequality operator.
	 * @param other ut_message to compare to
	 * @return Boolean result of comparison
	 */
	bool operator!=(ut_message const& other) const;

private:
	friend class cereal::access;

	template <typename Archive>
	void cerealize(Archive& ar);

	payload_type m_data;
};


/**
 * Get UT message variant type corresponding to UT message parameters.
 * @tparam HeaderAlignment Alignment of header in bits
 * @tparam SubwordType Type of subword which's width corresponds to the messages alignment
 * @tparam Dictionary Dictionary of instructions
 */
template <size_t HeaderAlignment, typename SubwordType, typename Dictionary>
struct to_ut_message_variant;


/**
 * Get the largest UT message size in bits for a given set of UT messsage parameters.
 * @tparam HeaderAlignment Alignment of header in bits
 * @tparam SubwordType Type of subword which's width corresponds to the messages alignment
 * @tparam Dictionary Dictionary of instructions
 */
template <size_t HeaderAlignment, typename SubwordType, typename Dictionary>
struct largest_ut_message_size;

/**
 * Get the header width corresponding to a given set of UT messsage parameters.
 * @tparam HeaderAlignment Alignment of header in bits
 * @tparam Dictionary Dictionary of instructions
 */
template <size_t HeaderAlignment, typename Dictionary>
struct ut_message_header_width;

} // namespace hxcomm

#include "hxcomm/common/utmessage.tcc"
