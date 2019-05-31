#include "flange/simulator_control.h"

extern "C"
{
#include <unistd.h>
}

#include "flange/simulator_control_if.h"

namespace flange {

SimulatorControl::SimulatorControl() :
    m_current_clk(0),
    m_runnable(false),
    m_pause_after_next_event_from_sim(false),
    m_terminate_asap(false)
{}

void SimulatorControl::set_runnable(bool state)
{
	RCF::Lock lock(m_service_lock);
	m_runnable = state;
}

bool SimulatorControl::get_runnable() const
{
	RCF::Lock lock(m_service_lock);
	return m_runnable;
}

void SimulatorControl::issue_terminate()
{
	RCF::Lock lock(m_service_lock);
	m_terminate_asap = true;
}

void SimulatorControl::push_event(SimulatorEvent const& se)
{
	RCF::Lock lock(m_service_lock);
	m_to_sim.push_back(se);
}

bool SimulatorControl::received_data_available() const
{
	RCF::Lock lock(m_service_lock);
	return !m_from_sim.empty();
}

SimulatorEvent SimulatorControl::pop_front()
{
	RCF::Lock lock(m_service_lock);
	if (m_from_sim.empty()) {
		throw std::runtime_error("No data from sim available!");
	}
	SimulatorEvent front = m_from_sim.front();
	m_from_sim.pop_front();
	return front;
}

std::deque<SimulatorEvent> SimulatorControl::pop_events()
{
	RCF::Lock lock(m_service_lock);
	if (m_from_sim.empty()) {
		throw std::runtime_error("No data from sim available!");
	}
	std::deque<SimulatorEvent> ret = m_from_sim;
	m_from_sim.clear();
	return ret;
}

SimulatorEvent::clk_t SimulatorControl::current_clk() const
{
	RCF::Lock lock(m_service_lock);
	return m_current_clk;
}

} // namespace flange
