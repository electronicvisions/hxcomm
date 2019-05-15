namespace hxcomm {

template <typename UTMessageParameter, typename WordQueueType>
Encoder<UTMessageParameter, WordQueueType>::Encoder(word_queue_type& word_queue) :
    m_buffer(),
    m_buffer_filling_level(0),
    m_word_queue(word_queue)
{}

template <typename UTMessageParameter, typename WordQueueType>
template <class MessageType>
typename std::enable_if<detail::is_ut_message<MessageType>::value, void>::type
Encoder<UTMessageParameter, WordQueueType>::operator()(MessageType const& message)
{
	static_assert(
	    hate::is_in_type_list<
	        typename MessageType::instruction_type, typename UTMessageParameter::Dictionary>::value,
	    "Message type is not in dictionary.");

	m_buffer_filling_level += message.word_width;

	m_buffer |= (buffer_type(message.get_raw()) << (buffer_size - m_buffer_filling_level));

	size_t const num_words_to_shift_out = m_buffer_filling_level / m_buffer.num_bits_per_word;

	assert(num_words_to_shift_out < m_buffer.num_words);

	auto it = m_buffer.to_array().crbegin();
	auto const it_end = it + num_words_to_shift_out;
	for (; it < it_end; ++it) {
		m_word_queue.push(*it);
	}

	m_buffer.shift_words_left(num_words_to_shift_out);
	m_buffer_filling_level -= (num_words_to_shift_out * m_buffer.num_bits_per_word);
}

template <typename UTMessageParameter, typename WordQueueType>
template <typename Iterable>
typename std::enable_if<!detail::is_ut_message<Iterable>::value, void>::type
Encoder<UTMessageParameter, WordQueueType>::operator()(Iterable const& messages)
{
	for (auto it = messages.cbegin(); it < messages.cend(); ++it) {
		boost::apply_visitor([this](auto&& m) { this->operator()(m); }, *it);
	}
}

template <typename UTMessageParameter, typename WordQueueType>
void Encoder<UTMessageParameter, WordQueueType>::flush()
{
	if (m_buffer_filling_level) {
		// set comma
		m_buffer.set(buffer_size - m_buffer_filling_level - 1);
		// shift out word to queue
		m_word_queue.push(m_buffer.to_array().back());
		// empty buffer
		m_buffer.shift_words_left(1);
		m_buffer_filling_level = 0;
	}
}

} // namespace hxcomm
