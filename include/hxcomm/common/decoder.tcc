#include "hxcomm/common/logger.h"
#include <sstream>

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
    m_coroutine(std::bind(
        &Decoder<UTMessageParameter, MessageQueueType, Listener...>::coroutine,
        this,
        std::placeholders::_1)),
    m_logger(log4cxx::Logger::getLogger("hxcomm.Decoder"))
{}

template <typename UTMessageParameter, typename MessageQueueType, typename... Listener>
Decoder<UTMessageParameter, MessageQueueType, Listener...>::Decoder(
    Decoder& other, message_queue_type& message_queue, Listener&... listener) :
    m_buffer(other.m_buffer),
    m_buffer_filling_level(other.m_buffer_filling_level),
    m_message_queue(message_queue),
    m_listener(listener...),
    m_coroutine(std::bind(
        &Decoder<UTMessageParameter, MessageQueueType, Listener...>::coroutine,
        this,
        std::placeholders::_1)),
    m_logger(log4cxx::Logger::getLogger("hxcomm.Decoder"))
{
	other.m_buffer.reset();
	other.m_buffer_filling_level = 0;
}

template <typename UTMessageParameter, typename MessageQueueType, typename... Listener>
void Decoder<UTMessageParameter, MessageQueueType, Listener...>::operator()(word_type const word)
{
	HXCOMM_LOG_TRACE(
	    m_logger, "operator(): Got PHY word to decode: 0x"
	                  << std::setfill('0') << std::setw(sizeof(word_type) * 2) << std::hex << word);
	m_coroutine(word);
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
		HXCOMM_LOG_DEBUG(
		    m_logger, "operator(): Got PHY word to decode: " << std::showbase << std::setfill('0')
		                                                     << std::setw(sizeof(word_type) * 2)
		                                                     << std::hex << *it);
	}
	std::copy(begin, end, typename coroutine_type::push_type::iterator(&m_coroutine));
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
	HXCOMM_LOG_DEBUG(m_logger, "decode_message(): Decoded UT message: " << message);
	boost::fusion::for_each(m_listener, [message](auto& l) { l(message); });
	m_message_queue.push(std::move(message));
}

template <typename UTMessageParameter, typename MessageQueueType, typename... Listener>
template <size_t... Header>
void Decoder<UTMessageParameter, MessageQueueType, Listener...>::decode_message_table_generator(
    size_t const header, std::index_sequence<Header...>)
{
	constexpr static auto function_table = std::array{&Decoder::decode_message<Header>...};
	(this->*function_table[header])();
}

template <typename UTMessageParameter, typename MessageQueueType, typename... Listener>
void Decoder<UTMessageParameter, MessageQueueType, Listener...>::coroutine(
    typename coroutine_type::pull_type& source)
{
	while (true) {
		if (m_buffer_filling_level < header_size) {
			while (has_leading_comma(source.get())) {
				source();
			}
			shift_in_buffer(source.get());
			if constexpr (header_size > num_bits_word) {
				while (m_buffer_filling_level < header_size) {
					source();
					shift_in_buffer(source.get());
				}
			}
		}
		size_t const header = decode_header();
		size_t const message_size = get_message_size(header);
		while (m_buffer_filling_level < message_size) {
			source();
			shift_in_buffer(source.get());
		}
		decode_message(header);
		m_buffer_filling_level -= message_size;
		// if remaining tail of word has a leading comma, drop the word.
		if (m_buffer_filling_level) {
			if (m_buffer.test(m_buffer_filling_level - 1)) {
				m_buffer_filling_level -= (m_buffer_filling_level % num_bits_word);
			}
		}
		if (m_buffer_filling_level < header_size) {
			source();
		}
	}
}

} // namespace hxcomm
