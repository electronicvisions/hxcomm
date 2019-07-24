#pragma once

#include <deque>
#include "RCF/RCF.hpp"

#include "flange/simulator_dpi.h"
#include "flange/simulator_event.h"

namespace flange {

/** @brief SimulatorControl is an I/O-queue interface class.
 *
 *  @note All functions use mutexes (or similar means) to
 *  support multi-threaded access to the class.
 */
class SimulatorControl
{
public:
	SimulatorControl();

	/// Return true if class is in runnable state
	bool get_runnable() const;

	/// Set runnable state to target value
	void set_runnable(bool state);

	/// Set terminate state
	void issue_terminate();

	/// Set reset state for count cycle(s).
	void issue_reset(SimulatorEvent::clk_t count);

	/// Add SimulatorEvent to "to_sim" queue
	void push_event(SimulatorEvent const& se);

	/// Check for events in "from_sim" queue
	bool received_data_available() const;

	/// Pop and return first event in "from_sim" queue
	SimulatorEvent pop_front();

	/// Pop and return all events in "from_sim" queue
	std::deque<SimulatorEvent> pop_events();

	/// Return current system time in "simulator clocks"
	SimulatorEvent::clk_t current_clk() const;

private:
	/// C-API access functions
	friend dpi_handle_t ::dpi_comm_init();
	friend void ::dpi_comm_shutdown(dpi_handle_t handle);
	friend bool ::dpi_comm_tx(dpi_handle_t handle, uint64_t tx_data);
	friend bool ::dpi_comm_rx(
	    dpi_handle_t handle, bool rx_ready, bool* terminate, bool* reset, uint64_t* rx_data);

	mutable RCF::Mutex m_service_lock;
	std::deque<SimulatorEvent> m_to_sim;
	std::deque<SimulatorEvent> m_from_sim;
	SimulatorEvent::clk_t m_current_clk;
	bool m_runnable;
	bool m_pause_after_next_event_from_sim;
	bool m_terminate_asap;
	SimulatorEvent::clk_t m_reset;
};

} // namespace flange
