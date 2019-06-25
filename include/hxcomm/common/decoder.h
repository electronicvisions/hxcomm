#pragma once
#include <climits>
#include <boost/coroutine2/all.hpp>
#include <boost/function.hpp>
#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/tuple.hpp>

#include "hxcomm/common/utmessage.h"

namespace hxcomm {

/**
 * Decoder sequentially decoding a stream of words into a UT message queue.
 * The queue is expected to provide a push function.
 * @tparam UTMessageParameter UT message parameter
 * @tparam MessageQueueType Queue type used for storing decoded UT messages in
 * @tparam Listener An arbitrary number of message listeners to be invoked for each decoded message
 * using their operator()
 */
template <
    typename UTMessageParameter,
    typename MessageQueueType,
    typename... Listener>
class Decoder
{
public:
	typedef typename ToUTMessageVariant<
	    UTMessageParameter::HeaderAlignment,
	    typename UTMessageParameter::SubwordType,
	    typename UTMessageParameter::PhywordType,
	    typename UTMessageParameter::Dictionary>::type receive_message_type;

	typedef MessageQueueType message_queue_type;
	typedef typename UTMessageParameter::PhywordType word_type;

	/**
	 * Initialize decoder with a reference to a message queue to push decoded messages to and
	 * references to message listeners to invoke on decoded messages.
	 * @param message_queue Reference to message queue
	 * @param listener List of references to message listeners
	 */
	Decoder(message_queue_type& message_queue, Listener&... listener);

	/**
	 * Decode a word.
	 * During the process multiple messages might be decoded and pushed to the message queue.
	 * @param word Word to decode
	 */
	void operator()(word_type word);

	/**
	 * Decode a ensemble of words from an iterable.
	 * During the process multiple messages might be decoded and pushed to the message queue.
	 * The iterable has to define a cbegin and cend function for iteration.
	 * @tparam Iterable Iterable type of words
	 * @param iterable Iterable of words to decode
	 */
	template <typename Iterable>
	void operator()(Iterable const& iterable);

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


	static constexpr size_t header_size = UTMessageHeaderWidth<
	    UTMessageParameter::HeaderAlignment,
	    typename UTMessageParameter::Dictionary>::value;

	typedef hate::bitset<buffer_size, word_type> buffer_type;

	buffer_type m_buffer;

	size_t m_buffer_filling_level;

	message_queue_type& m_message_queue;

	boost::fusion::tuple<Listener&...> m_listener;

	/**
	 * Test, if word has a leading comma, i.e. the highest bit is set to true.
	 * @param word Word to test
	 * @return Boolean test result
	 */
	bool has_leading_comma(word_type word) const;

	/**
	 * Shift word into buffer.
	 * @param word Word to shift in
	 */
	void shift_in_buffer(word_type word);

	/**
	 * Decode UT message header.
	 * @return Header
	 */
	size_t decode_header() const;

	/**
	 * Get UT message size for specified header.
	 * @param header Header to lookup UT message size for
	 */
	constexpr size_t get_message_size(size_t header);

	/**
	 * Decode UT message corresponding to header from buffer and push into message queue.
	 * @param header Header to decode UT message for
	 */
	void decode_message(size_t header);

	template <size_t... Header>
	void decode_message_table_generator(size_t header, std::index_sequence<Header...>);

	typedef boost::coroutines2::coroutine<word_type> coroutine_type;

	typename coroutine_type::push_type m_coroutine;

	void coroutine(typename coroutine_type::pull_type& source);
};

} // namespace hxcomm

#include "hxcomm/common/decoder.tcc"
