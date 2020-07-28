#include "hxcomm/common/logger.h"

namespace hxcomm {

template <typename UTMessageParameter, typename WordQueueType>
Encoder<UTMessageParameter, WordQueueType>::Encoder(word_queue_type& word_queue) :
    m_buffer(),
    m_buffer_filling_level(0),
    m_word_queue(word_queue),
    m_logger(log4cxx::Logger::getLogger("hxcomm.Encoder"))
{}

template <typename UTMessageParameter, typename WordQueueType>
Encoder<UTMessageParameter, WordQueueType>::Encoder(Encoder& other, word_queue_type& word_queue) :
    m_buffer(other.m_buffer),
    m_buffer_filling_level(other.m_buffer_filling_level),
    m_word_queue(word_queue),
    m_logger(log4cxx::Logger::getLogger("hxcomm.Encoder"))
{
	other.m_buffer.reset();
	other.m_buffer_filling_level = 0;
}

template <typename UTMessageParameter, typename WordQueueType>
template <class MessageType>
typename std::enable_if<detail::IsUTMessage<MessageType>::value, void>::type
Encoder<UTMessageParameter, WordQueueType>::operator()(MessageType const& message)
{
	HXCOMM_LOG_DEBUG(m_logger, "operator(): Got UT message: " << message);

	static_assert(
	    hate::is_in_type_list<
	        typename MessageType::instruction_type, typename UTMessageParameter::Dictionary>::value,
	    "Message type is not in dictionary.");

	m_buffer_filling_level += message.word_width;

	m_buffer |= (buffer_type(message.get_raw()) << (buffer_size - m_buffer_filling_level));

	size_t const num_words_to_shift_out = m_buffer_filling_level / m_buffer.num_bits_per_word;
	m_buffer_filling_level %= m_buffer.num_bits_per_word;

	auto it = m_buffer.to_array().crbegin();
	auto const it_end = it + num_words_to_shift_out;
	for (; it < it_end; ++it) {
		HXCOMM_LOG_TRACE(
		    m_logger, "operator(): Encoded PHY word: " << std::showbase << std::setfill('0')
		                                               << std::setw(sizeof(word_type) * 2)
		                                               << std::hex << *it);
		m_word_queue.push(*it);
	}

	m_buffer.shift_words_left(num_words_to_shift_out);
}

template <typename UTMessageParameter, typename WordQueueType>
template <typename Iterable>
typename std::enable_if<!detail::IsUTMessage<Iterable>::value, void>::type
Encoder<UTMessageParameter, WordQueueType>::operator()(Iterable const& messages)
{
	for (auto const& message : messages) {
		boost::apply_visitor([this](auto const& m) { this->operator()(m); }, message);
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
		HXCOMM_LOG_TRACE(
		    m_logger, "flush(): Encoded PHY word: " << std::showbase << std::setfill('0')
		                                            << std::setw(sizeof(word_type) * 2) << std::hex
		                                            << m_buffer.to_array().back());
		// empty buffer
		m_buffer.shift_words_left(1);
		m_buffer_filling_level = 0;
	}
}

} // namespace hxcomm
