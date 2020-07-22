#pragma once
#include <atomic>
#include <queue>
#include <stdint.h>
#include <thread>
#include <tbb/concurrent_queue.h>

#include "hxcomm/common/decoder.h"
#include "hxcomm/common/encoder.h"
#include "hxcomm/common/utmessage.h"

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
	 * @param messages Messages to add
	 */
	void add(std::vector<send_message_type> const& messages);

	/**
	 * Send messages in send queue by moving to the intermediate queue.
	 */
	void commit();

	/**
	 * Receive a single UT message.
	 * @throws std::runtime_error On empty message queue
	 * @return Received message
	 */
	receive_message_type receive();

	/**
	 * Try to receive a single UT message.
	 * @param message Message to receive to
	 * @return Boolean value whether receive was successfule
	 */
	bool try_receive(receive_message_type& message);

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

	typedef tbb::concurrent_queue<receive_message_type> receive_queue_type;
	receive_queue_type m_receive_queue;

	Decoder<UTMessageParameter, receive_queue_type> m_decoder;

	std::atomic<bool> m_run_receive;

	std::thread m_worker_receive;
	void work_receive();
};

} // namespace hxcomm

#include "hxcomm/common/loopbackconnection.tcc"
