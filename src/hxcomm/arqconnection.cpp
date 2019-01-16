#include <unistd.h>

#include <array>

#include "hxcomm/arqconnection.h"
#include "sctrltp/ARQFrame.h"

namespace hxcomm {

static constexpr std::array<int, 6> timeout_intervals_us = {10, 100, 1000, 10000, 100000, 1000000};

ARQConnection::ARQConnection(ip_address_type const fpgaip) : m_arq_stream(fpgaip.to_string()) {}

void ARQConnection::send_one(ut_message_t const message, bool do_flush)
{
	sctrltp::packet pkt;
	pkt.pid = pid;
	pkt.len = 1;
	pkt.pdu[0] = message;

	m_arq_stream.send(pkt, sctrltp::ARQStream::NOTHING);
	if (do_flush) {
		flush();
	}
}

ut_message_t ARQConnection::receive_one()
{
	for (size_t wait_counter = 0; !m_arq_stream.received_packet_available(); wait_counter++) {
		if (wait_counter == timeout_intervals_us.size()) {
			throw std::runtime_error("Timeout while waiting for ARQ packet");
		}
		usleep(timeout_intervals_us[wait_counter]);
	}

	sctrltp::packet received_packet;
	m_arq_stream.receive(received_packet);

	return received_packet.pdu[0];
}

void ARQConnection::flush()
{
	m_arq_stream.flush();
	for (size_t wait_counter = 0; !m_arq_stream.all_packets_sent(); wait_counter++) {
		if (wait_counter == timeout_intervals_us.size()) {
			throw std::runtime_error("Timeout while sending ARQ packet(s)");
		}
		usleep(timeout_intervals_us[wait_counter]);
	}
}

bool ARQConnection::receive_data_available()
{
	return m_arq_stream.received_packet_available();
}

} // namespace hxcomm
