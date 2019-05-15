namespace hxcomm {

namespace detail {

template <size_t HeaderAlignment, typename SubwordType, typename PhywordType, typename Dictionary>
struct ut_message_sizes;

template <size_t HeaderAlignment, typename SubwordType, typename PhywordType, typename... Ts>
struct ut_message_sizes<HeaderAlignment, SubwordType, PhywordType, hate::type_list<Ts...> >
{
	constexpr static std::array<size_t, sizeof...(Ts)> value = {
	    ut_message<HeaderAlignment, SubwordType, PhywordType, hate::type_list<Ts...>, Ts>::word_width...};
};

} // namespace detail


template <
    typename UTMessageParameter,
    typename MessageQueueType,
    typename... Listener>
Decoder<UTMessageParameter, MessageQueueType, Listener...>::Decoder(
    message_queue_type& message_queue, Listener&... listener) :
    m_buffer(),
    m_buffer_filling_level(0),
    m_message_queue(message_queue),
    m_listener(listener...),
    m_coroutine(std::bind(
        &Decoder<UTMessageParameter, MessageQueueType, Listener...>::coroutine,
        this,
        std::placeholders::_1))
{}

template <
    typename UTMessageParameter,
    typename MessageQueueType,
    typename... Listener>
void Decoder<UTMessageParameter, MessageQueueType, Listener...>::operator()(
    word_type const word)
{
	m_coroutine(word);
}

template <
    typename UTMessageParameter,
    typename MessageQueueType,
    typename... Listener>
template <typename Iterable>
void Decoder<UTMessageParameter, MessageQueueType, Listener...>::operator()(
    Iterable const& iterable)
{
	std::copy(iterable.cbegin(), iterable.cend(), begin(m_coroutine));
}

template <
    typename UTMessageParameter,
    typename MessageQueueType,
    typename... Listener>
bool Decoder<UTMessageParameter, MessageQueueType, Listener...>::has_leading_comma(
    word_type const word) const
{
	constexpr word_type comma_mask = (static_cast<word_type>(1) << (num_bits_word - 1));
	return (word & comma_mask);
}

template <
    typename UTMessageParameter,
    typename MessageQueueType,
    typename... Listener>
void Decoder<UTMessageParameter, MessageQueueType, Listener...>::shift_in_buffer(
    word_type const word)
{
	m_buffer.shift_words_left(1);
	m_buffer |= hate::bitset<num_bits_word, typename buffer_type::word_type>(word);
	m_buffer_filling_level += num_bits_word;
}

template <
    typename UTMessageParameter,
    typename MessageQueueType,
    typename... Listener>
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

template <
    typename UTMessageParameter,
    typename MessageQueueType,
    typename... Listener>
constexpr size_t
Decoder<UTMessageParameter, MessageQueueType, Listener...>::get_message_size(
    size_t const header)
{
	return detail::ut_message_sizes<
	    UTMessageParameter::HeaderAlignment, typename UTMessageParameter::SubwordType, typename UTMessageParameter::PhywordType,
	    typename UTMessageParameter::Dictionary>::value[header];
}

template <
    typename UTMessageParameter,
    typename MessageQueueType,
    typename... Listener>
void Decoder<UTMessageParameter, MessageQueueType, Listener...>::decode_message(
    size_t const header)
{
	decode_message_table_generator(
	    header, std::make_index_sequence<
	                hate::type_list_size<typename UTMessageParameter::Dictionary>::value>());
}

template <
    typename UTMessageParameter,
    typename MessageQueueType,
    typename... Listener>
template <size_t... Header>
void Decoder<UTMessageParameter, MessageQueueType, Listener...>::
    decode_message_table_generator(size_t const header, std::index_sequence<Header...>)
{
	constexpr static std::array<
	    void (*)(
	        size_t&, buffer_type const&, message_queue_type&, boost::fusion::tuple<Listener&...>),
	    sizeof...(Header)>
	    function_table{[](size_t& filling_level, buffer_type const& buffer,
	                      message_queue_type& queue, boost::fusion::tuple<Listener&...> listener) {
		    typedef ut_message<
		        UTMessageParameter::HeaderAlignment, typename UTMessageParameter::SubwordType, typename UTMessageParameter::PhywordType,
		        typename UTMessageParameter::Dictionary,
		        typename hate::index_type_list_by_integer<
		            Header, typename UTMessageParameter::Dictionary>::type>
		        ut_message_t;
		    ut_message_t message(typename ut_message_t::payload_type(
		        buffer >> (filling_level - ut_message_t::word_width)));
		    filling_level -= ut_message_t::word_width;
		    // if remaining tail of word has a leading comma, drop the word.
		    if (filling_level) {
			    if (buffer.test(filling_level - 1)) {
				    filling_level -= (filling_level % num_bits_word);
			    }
		    }
		    boost::fusion::for_each(listener, [message](auto& l) { l(message); });
		    queue.push(std::move(message));
	    }...};

	function_table[header](m_buffer_filling_level, m_buffer, m_message_queue, m_listener);
}

template <
    typename UTMessageParameter,
    typename MessageQueueType,
    typename... Listener>
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
		if (m_buffer_filling_level < header_size) {
			source();
		}
	}
}

} // namespace hxcomm
