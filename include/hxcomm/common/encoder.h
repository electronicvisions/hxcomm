#pragma once
#include "hxcomm/common/utmessage.h"
#include <climits>
#include <stdint.h>

namespace log4cxx {
class Logger;
} // namespace log4cxx

namespace hxcomm {

namespace detail {

template <typename... T>
struct IsUTMessage : public std::false_type
{};

template <
    size_t HeaderAlignment,
    typename SubwordType,
    typename PhywordType,
    typename Dictionary,
    typename Instruction>
struct IsUTMessage<UTMessage<HeaderAlignment, SubwordType, PhywordType, Dictionary, Instruction>>
    : public std::true_type
{};

} // namespace detail

/**
 * Encoder sequentially encoding UT messages to a word queue.
 * The queue is expected to provide a push function for single words.
 * @tparam UTMessageParameter UT message parameter
 * @tparam WordQueueType Queue type to push words to
 */
template <typename UTMessageParameter, typename WordQueueType>
class Encoder
{
public:
	typedef typename ToUTMessageVariant<
	    UTMessageParameter::HeaderAlignment,
	    typename UTMessageParameter::SubwordType,
	    typename UTMessageParameter::PhywordType,
	    typename UTMessageParameter::Dictionary>::type send_message_type;

	typedef typename UTMessageParameter::PhywordType word_type;
	typedef WordQueueType word_queue_type;

	/**
	 * Initialize encoder with a reference to a word queue to push encoded words to.
	 * @param word_queue Reference to word queue
	 */
	Encoder(word_queue_type& word_queue);

	/**
	 * Initialize encoder with a reference to a word queue to push encoded words to and a reference
	 * encoder from which to take existing buffer state.
	 * @param word_queue Reference to word queue
	 * @param other Reference encoder
	 */
	Encoder(Encoder& other, word_queue_type& word_queue);

	/**
	 * Encode a message.
	 * During the process multiple words might be produced and pushed to the word queue.
	 * @tparam MessageType Type of message to encode
	 * @param message Message to encode
	 */
	template <typename MessageType>
	typename std::enable_if<detail::IsUTMessage<MessageType>::value, void>::type operator()(
	    MessageType const& message);

	/**
	 * Encode multiple messages.
	 * During the process multiple words might be produced and pushed to the word queue.
	 * The implementation requires messages to have a cbegin and cend function for iteration.
	 * The containers' entries are to be message variants.
	 * @tparam InputIterator Iterator to UTMessage variant sequence
	 * @param begin Iterator to beginning of message sequence
	 * @param end Iterator to end of message sequence
	 */
	template <typename InputIterator>
	void operator()(InputIterator const& begin, InputIterator const& end);

	/**
	 * Flush the possibly partially filled head-word of the buffer to the queue.
	 * Appends a comma to the word before pushing.
	 */
	void flush();

private:
	static constexpr size_t num_bits_word = sizeof(word_type) * CHAR_BIT;

	static constexpr size_t buffer_size = hate::math::round_up_to_multiple(
	                                          LargestUTMessageSize<
	                                              UTMessageParameter::HeaderAlignment,
	                                              typename UTMessageParameter::SubwordType,
	                                              typename UTMessageParameter::PhywordType,
	                                              typename UTMessageParameter::Dictionary>::value,
	                                          num_bits_word) +
	                                      num_bits_word;

	typedef hate::bitset<buffer_size, word_type> buffer_type;

	buffer_type m_buffer;
	size_t m_buffer_filling_level;

	word_queue_type& m_word_queue;

	log4cxx::Logger* m_logger;
};

} // namespace hxcomm

#include "hxcomm/common/encoder.tcc"
