#include "flange/simulator_client.h"

#include <array>
#include <unistd.h>

#include "flange/exception.h"
#include "flange/simulator_control_if.h"

namespace flange {

static constexpr std::array<useconds_t, 6> timeout_intervals_us = {10,    100,    1000,
                                                                   10000, 100000, 1000000};

struct SimulatorClient::Impl
{
	Impl(ip_t ip, port_t port) : m_rcf(), m_sim(RCF::TcpEndpoint(ip, port)) {}

	RCF::RcfInitDeinit m_rcf;
	flange::RcfClient<flange::I_SimulatorControl> m_sim;
};

SimulatorClient::SimulatorClient(ip_t ip, port_t port) : m_impl(std::make_unique<Impl>(ip, port)) {}

SimulatorClient::~SimulatorClient()
{}

void SimulatorClient::send(word_type const word)
{
	SimulatorEvent event;
	event.timestamp = SimulatorEvent::timestamp_asap; // back-to-back
	event.data = word;
	event.event_type = SimulatorEvent::event_types::AL_DATA;
	m_impl->m_sim.push_event(event);
}

SimulatorClient::word_type SimulatorClient::receive()
{
	for (size_t wait_counter = 0; !m_impl->m_sim.received_data_available(); wait_counter++) {
		if (wait_counter == timeout_intervals_us.size()) {
			throw timeout("Timeout while waiting for simulation response");
		}
		usleep(timeout_intervals_us[wait_counter]);
	}
	SimulatorEvent received_event = m_impl->m_sim.pop_front();
	return received_event.data;
}

void SimulatorClient::set_runnable(bool const value)
{
	m_impl->m_sim.set_runnable(value);
}

bool SimulatorClient::get_runnable()
{
	return m_impl->m_sim.get_runnable();
}

bool SimulatorClient::receive_data_available()
{
	return m_impl->m_sim.received_data_available();
}

SimulatorEvent::clk_t SimulatorClient::get_current_time()
{
	return m_impl->m_sim.current_clk();
}

void SimulatorClient::set_remote_timeout(int const timeout)
{
	m_impl->m_sim.getClientStub().setRemoteCallTimeoutMs(timeout);
}

void SimulatorClient::issue_terminate()
{
	m_impl->m_sim.issue_terminate();
}

} // namespace flange
