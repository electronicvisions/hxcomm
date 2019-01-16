#pragma once
#include <stdint.h>

#include "sctrltp/ARQStream.h"

#include "hxcomm/utmessage.h"

namespace hxcomm {

/**
 * HostARQ connection class.
 * Establish and hold HostARQ connection to FPGA.
 * Provide convenience functions for sending and receiving UT messages.
 */
class ARQConnection
{
public:
	typedef sctrltp::ARQStream::ip_t ip_address_type;

	/**
	 * Create connection to FPGA.
	 * @param fpgaip IP-address of FPGA
	 */
	ARQConnection(ip_address_type const fpgaip);

	/**
	 * Send a packet containing a single UT message.
	 * On do_flush, the HostARQ buffer is emptied afterwards.
	 * @param message UT message to send
	 * @param do_flush Control whether to flush the HostARQ buffer
	 */
	void send_one(ut_message_t message, bool do_flush = true);

	/**
	 * Receive a packet containing a single UT message.
	 * @return Received UT message
	 * @throws std::runtime_error On timeout of 1s waiting for ARQ packet
	 */
	ut_message_t receive_one();

	/**
	 * Flush HostARQ buffer, i.e. process all packets currently in the buffer.
	 * @throws std::runtime_error On timeout of 1s waiting for processing of all packets
	 */
	void flush();

	/**
	 * Check whether packets to be received are available.
	 * @return Boolean value
	 */
	bool receive_data_available();

private:
	static constexpr uint16_t pid = 0x0010; // HostARQ UT packet type
	sctrltp::ARQStream m_arq_stream;
};

} // namespace hxcomm
