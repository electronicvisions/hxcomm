#pragma once
#include <climits>
#include <stdint.h>

#include "hxcomm/common/utmessage.h"

namespace hxcomm {

namespace detail {

template <typename... T>
struct is_ut_message : public std::false_type
{};

template <size_t HeaderAlignment, typename SubwordType, typename Dictionary, typename Instruction>
struct is_ut_message<ut_message<HeaderAlignment, SubwordType, Dictionary, Instruction>>
    : public std::true_type
{};

} // namespace detail

/**
 * Encoder sequentially encoding UT messages to a word queue.
 * The queue is expected to provide a push function for single words.
 * @tparam UTMessageParameter UT message parameter
 * @tparam WordType Word type of word queue
 * @tparam Derived WordQueueType Queue type to push words to
 */
template <typename UTMessageParameter, typename WordType, typename WordQueueType>
class Encoder
{
public:
	typedef typename to_ut_message_variant<
	    UTMessageParameter::HeaderAlignment,
	    typename UTMessageParameter::SubwordType,
	    typename UTMessageParameter::Dictionary>::type send_message_type;

	typedef WordType word_type;
	typedef WordQueueType word_queue_type;

	/**
	 * Initialize encoder with a reference to a word queue to push encoded words to.
	 * @param word_queue Reference to word queue
	 */
	Encoder(word_queue_type& word_queue);

	/**
	 * Encode a message.
	 * During the process multiple words might be produced and pushed to the word queue.
	 * @tparam MessageType Type of message to encode
	 * @param message Message to encode
	 */
	template <typename MessageType>
	typename std::enable_if<detail::is_ut_message<MessageType>::value, void>::type operator()(
	    MessageType const& message);

	/**
	 * Encode multiple messages.
	 * During the process multiple words might be produced and pushed to the word queue.
	 * The implementation requires messages to have a cbegin and cend function for iteration.
	 * The containers' entries are to be message variants.
	 * @tparam Iterable Iterable container storing message variants
	 * @param messages Message container to encode iteratively
	 */
	template <typename Iterable>
	typename std::enable_if<!detail::is_ut_message<Iterable>::value, void>::type operator()(
	    Iterable const& messages);

	/**
	 * Flush the possibly partially filled head-word of the buffer to the queue.
	 * Appends a comma to the word before pushing.
	 */
	void flush();

private:
	static constexpr size_t num_bits_word = sizeof(word_type) * CHAR_BIT;

	static constexpr size_t buffer_size = hate::math::round_up_to_multiple(
	                                          largest_ut_message_size<
	                                              UTMessageParameter::HeaderAlignment,
	                                              typename UTMessageParameter::SubwordType,
	                                              typename UTMessageParameter::Dictionary>::value,
	                                          num_bits_word) +
	                                      num_bits_word;

	typedef hate::bitset<buffer_size, word_type> buffer_type;

	buffer_type m_buffer;
	size_t m_buffer_filling_level;

	word_queue_type& m_word_queue;
};

} // namespace hxcomm

#include "hxcomm/common/encoder.tcc"