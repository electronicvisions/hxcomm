#include "hxcomm/common/logger.h"
#include <sstream>
#include <type_traits>
#include <boost/fusion/algorithm.hpp>

namespace hxcomm {

namespace detail {

template <size_t HeaderAlignment, typename SubwordType, typename PhywordType, typename Dictionary>
struct UTMessageSizes;

template <size_t HeaderAlignment, typename SubwordType, typename PhywordType, typename... Ts>
struct UTMessageSizes<HeaderAlignment, SubwordType, PhywordType, hate::type_list<Ts...> >
{
	constexpr static std::array<size_t, sizeof...(Ts)> value = {
	    UTMessage<HeaderAlignment, SubwordType, PhywordType, hate::type_list<Ts...>, Ts>::
	        word_width...};
};

} // namespace detail


template <typename UTMessageParameter, typename MessageQueueType, typename... Listener>
Decoder<UTMessageParameter, MessageQueueType, Listener...>::Decoder(
    message_queue_type& message_queue, Listener&... listener) :
    m_buffer(),
    m_buffer_filling_level(0),
    m_message_queue(message_queue),
    m_listener(listener...),
    m_logger(log4cxx::Logger::getLogger("hxcomm.Decoder")),
    m_state(State::dropping_leading_comma),
    m_current_header(0),
    m_current_message_size(0)
{}

template <typename UTMessageParameter, typename MessageQueueType, typename... Listener>
Decoder<UTMessageParameter, MessageQueueType, Listener...>::Decoder(
    Decoder& other, message_queue_type& message_queue, Listener&... listener) :
    m_buffer(other.m_buffer),
    m_buffer_filling_level(std::exchange(other.m_buffer_filling_level, 0)),
    m_message_queue(message_queue),
    m_listener(listener...),
    m_logger(log4cxx::Logger::getLogger("hxcomm.Decoder")),
    m_state(std::exchange(other.m_state, State::dropping_leading_comma)),
    m_current_header(std::exchange(other.m_current_header, 0)),
    m_current_message_size(std::exchange(other.m_current_message_size, 0))
{
	other.m_buffer.reset();
}

template <typename UTMessageParameter, typename MessageQueueType, typename... Listener>
void Decoder<UTMessageParameter, MessageQueueType, Listener...>::operator()(word_type const word)
{
	HXCOMM_LOG_TRACE(
	    m_logger, "operator(): Got PHY word to decode: 0x"
	                  << std::setfill('0') << std::setw(sizeof(word_type) * 2) << std::hex << word);
	switch (m_state) {
		case State::dropping_leading_comma: {
			/**
			 * If we currently drop leading commas, we continue to do so if we see a leading comma
			 * again.
			 */
			if (has_leading_comma(word)) {
				return;
			}
			/**
			 * If we don't see a leading comma anymore, we shift the word into the buffer.
			 */
			shift_in_buffer(word);
			if constexpr (header_size > num_bits_word) {
				/**
				 * Only if the size of a word is smaller than one header size, we need to continue
				 * filling words into the buffer until we have at least one header.
				 */
				if (m_buffer_filling_level < header_size) {
					m_state = State::filling_until_header_size;
					return;
				}
			}
			/**
			 * We have enough data to decode a header and directly continue to do so.
			 */
			goto decoding_header;
		}
		case State::filling_until_header_size: {
			/**
			 * We don't yet have a single header to decode, but know, that every word which in this
			 * state is processed will be shifted into the buffer.
			 */
			shift_in_buffer(word);
			/**
			 * If we still don't have enough data for decoding a single header, we continue to do so
			 * with the next processed word.
			 */
			if (m_buffer_filling_level < header_size) {
				return;
			}
			/**
			 * We have enough data to decode a header and directly continue to do so.
			 */
			goto decoding_header;
		}
		case State::filling_until_message_size: {
			/**
			 * We have decoded the header of the next message, but don't yet have enough data to
			 * decode the message and therefore shift this word into the buffer.
			 */
			shift_in_buffer(word);
			/**
			 * If we still don't have enough data for decoding the message, we continue to do shift
			 * the next processed word into the buffer and check again.
			 */
			if (m_buffer_filling_level < m_current_message_size) {
				return;
			}
			/**
			 * We have enough data to decode the message and directly continue to do so.
			 */
			goto decoding_message;
		}
		default: {
			throw std::logic_error("default should never be reached");
		}
	}

decoding_header:
	/**
	 * We decode the header from the data in the buffer and calculate the message's size.
	 */
	m_current_header = decode_header();
	m_current_message_size = get_message_size(m_current_header);
	/**
	 * If the buffer doesn't contain enough data for the message to be decoded, we will shift the
	 * next processed word into the buffer and check again.
	 */
	if (m_buffer_filling_level < m_current_message_size) {
		m_state = State::filling_until_message_size;
		return;
	}
	/**
	 * Otherwise we directly continue to decode the message.
	 */
decoding_message:
	/**
	 * We decode the message given by the header from the data in the buffer.
	 */
	decode_message(m_current_header);
	/**
	 * Now the buffer doesn't contain the just decoded message data anymore.
	 */
	m_buffer_filling_level -= m_current_message_size;
	/**
	 * If the remaining content of the last word in the buffer has a leading comma, we drop the
	 * word.
	 */
	if (m_buffer_filling_level) {
		if (m_buffer.test(m_buffer_filling_level - 1)) {
			m_buffer_filling_level -= (m_buffer_filling_level % num_bits_word);
		}
	}
	if (m_buffer_filling_level < header_size) {
		/**
		 * If we don't have enough data in the buffer to directly decode the next message's header,
		 * we fall back into being able to drop leading commas in processed words.
		 */
		m_state = State::dropping_leading_comma;
		return;
	} else {
		/**
		 * Otherwise we directly continue to decode the next message's header.
		 */
		goto decoding_header;
	}
}

template <typename UTMessageParameter, typename MessageQueueType, typename... Listener>
template <typename InputIterator>
void Decoder<UTMessageParameter, MessageQueueType, Listener...>::operator()(
    InputIterator const& begin, InputIterator const& end)
{
	typedef std::iterator_traits<InputIterator> iterator_traits;
	// Expect iterator value type to be send_message_type
	static_assert(std::is_same_v<typename iterator_traits::value_type, word_type>);

	// Expect the iterator category to satisfy input iterator category
	static_assert(
	    std::is_base_of_v<std::input_iterator_tag, typename iterator_traits::iterator_category>);

	for (auto it = begin; it != end; ++it) {
		operator()(*it);
	}
}

template <typename UTMessageParameter, typename MessageQueueType, typename... Listener>
bool Decoder<UTMessageParameter, MessageQueueType, Listener...>::has_leading_comma(
    word_type const word)
{
	constexpr word_type comma_mask = (static_cast<word_type>(1) << (num_bits_word - 1));
	return (word & comma_mask);
}

template <typename UTMessageParameter, typename MessageQueueType, typename... Listener>
void Decoder<UTMessageParameter, MessageQueueType, Listener...>::shift_in_buffer(
    word_type const word)
{
	m_buffer.shift_words_left(1);
	m_buffer |= hate::bitset<num_bits_word, typename buffer_type::word_type>(word);
	m_buffer_filling_level += num_bits_word;
}

template <typename UTMessageParameter, typename MessageQueueType, typename... Listener>
size_t Decoder<UTMessageParameter, MessageQueueType, Listener...>::decode_header() const
{
	size_t const header = static_cast<size_t>(
	    hate::bitset<header_size, size_t>((m_buffer >> (m_buffer_filling_level - header_size))));
	// expect no unknown UT message header in normal execution
	if (__builtin_expect(
	        header >= hate::type_list_size<typename UTMessageParameter::Dictionary>::value,
	        false)) {
		std::stringstream ss;
		ss << "Unknown UT message header: " << header;
		throw std::runtime_error(ss.str());
	}
	return header;
}

template <typename UTMessageParameter, typename MessageQueueType, typename... Listener>
constexpr size_t Decoder<UTMessageParameter, MessageQueueType, Listener...>::get_message_size(
    size_t const header)
{
	return detail::UTMessageSizes<
	    UTMessageParameter::HeaderAlignment, typename UTMessageParameter::SubwordType,
	    typename UTMessageParameter::PhywordType,
	    typename UTMessageParameter::Dictionary>::value[header];
}

template <typename UTMessageParameter, typename MessageQueueType, typename... Listener>
void Decoder<UTMessageParameter, MessageQueueType, Listener...>::decode_message(size_t const header)
{
	decode_message_table_generator(
	    header, std::make_index_sequence<
	                hate::type_list_size<typename UTMessageParameter::Dictionary>::value>());
}

namespace detail {

template <typename Queue, typename = void>
struct has_push : std::false_type
{};

template <typename Queue>
struct has_push<
    Queue,
    std::void_t<decltype((void (Queue::*)(typename Queue::value_type&&)) & Queue::push)>>
    : std::true_type
{};

} // namespace detail

template <typename UTMessageParameter, typename MessageQueueType, typename... Listener>
template <size_t Header>
void Decoder<UTMessageParameter, MessageQueueType, Listener...>::decode_message()
{
	typedef UTMessage<
	    UTMessageParameter::HeaderAlignment, typename UTMessageParameter::SubwordType,
	    typename UTMessageParameter::PhywordType, typename UTMessageParameter::Dictionary,
	    typename hate::index_type_list_by_integer<
	        Header, typename UTMessageParameter::Dictionary>::type>
	    ut_message_t;
	ut_message_t message(typename ut_message_t::payload_type(
	    m_buffer >> (m_buffer_filling_level - ut_message_t::word_width)));
	HXCOMM_LOG_TRACE(m_logger, "decode_message(): Decoded UT message: " << message);
	boost::fusion::for_each(m_listener, [message](auto& l) { l(message); });
	if constexpr (detail::has_push<MessageQueueType>::value) {
		m_message_queue.push(std::move(message));
	} else {
		m_message_queue.push_back(std::move(message));
	}
}

template <typename UTMessageParameter, typename MessageQueueType, typename... Listener>
template <size_t... Header>
void Decoder<UTMessageParameter, MessageQueueType, Listener...>::decode_message_table_generator(
    size_t const header, std::index_sequence<Header...>)
{
	constexpr static auto function_table = std::array{&Decoder::decode_message<Header>...};
	(this->*function_table[header])();
}

} // namespace hxcomm
