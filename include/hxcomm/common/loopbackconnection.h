#pragma once
#include "hxcomm/common/decoder.h"
#include "hxcomm/common/encoder.h"
#include "hxcomm/common/utmessage.h"
#include <atomic>
#include <queue>
#include <stdint.h>
#include <thread>
#include <vector>

namespace hxcomm {

/**
 * Loopback connection class. Committed packets are added to a intermediate packet queue in software
 * without alteration. Every committed message can be received in order.
 * The send and receive dictionary are therefore the same.
 * @tparam UTMessageParameter UT message parameter
 */
template <typename UTMessageParameter>
class LoopbackConnection
{
public:
	typedef UTMessageParameter ut_message_parameter_type;

	static constexpr size_t header_alignment = UTMessageParameter::HeaderAlignment;
	typedef typename UTMessageParameter::PhywordType phyword_type;
	typedef typename UTMessageParameter::SubwordType subword_type;
	typedef typename UTMessageParameter::Dictionary dictionary_type;
	typedef phyword_type subpacket_type;

	typedef typename ToUTMessageVariant<
	    UTMessageParameter::HeaderAlignment,
	    typename UTMessageParameter::SubwordType,
	    typename UTMessageParameter::PhywordType,
	    typename UTMessageParameter::Dictionary>::type send_message_type;
	typedef send_message_type receive_message_type;

	typedef std::vector<receive_message_type> receive_queue_type;

	/**
	 * Default construct loopback connection.
	 */
	LoopbackConnection();

	/**
	 * Destruct loopback connection joining all receive threads.
	 */
	~LoopbackConnection();

	/**
	 * Add a single UT message to the send queue.
	 * @param message Message to add
	 */
	void add(send_message_type const& message);

	/**
	 * Add multiple UT messages to the send queue.
	 * @tparam InputIterator Iterator type to sequence of messages to add
	 * @param begin Iterator to beginning of sequence
	 * @param end Iterator to end of sequence
	 */
	template <typename InputIterator>
	void add(InputIterator const& begin, InputIterator const& end);

	/**
	 * Send messages in send queue by moving to the intermediate queue.
	 */
	void commit();

	/**
	 * Receive all UT messages currently in the receive queue.
	 * @return Received messages
	 */
	receive_queue_type receive_all();

	/**
	 * Get whether the connection has no UT messages available to receive.
	 * @return Boolean value
	 */
	bool receive_empty() const;

private:
	std::mutex m_intermediate_queue_mutex;
	std::deque<subpacket_type> m_intermediate_queue;

	typedef std::queue<subpacket_type> send_queue_type;
	send_queue_type m_send_queue;

	Encoder<UTMessageParameter, send_queue_type> m_encoder;

	mutable std::mutex m_receive_queue_mutex;
	receive_queue_type m_receive_queue;

	Decoder<UTMessageParameter, receive_queue_type> m_decoder;

	std::atomic<bool> m_run_receive;

	std::thread m_worker_receive;
	void work_receive();
};

} // namespace hxcomm

#include "hxcomm/common/loopbackconnection.tcc"
