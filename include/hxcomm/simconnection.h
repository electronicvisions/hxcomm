#pragma once

#include "flange/simulator_client.h"

namespace hxcomm {

/**
 * Simulation connection class.
 * Establish and hold Simulation connection to FPGA.
 * Provide convenience functions for sending and receiving UT messages.
 */
class SimConnection : public flange::SimulatorClient
{
public:
	/**
	 * Create and start connection to simulation server.
	 * @param ip IP-address of simulation server
	 * @param port Port of simulation server
	 */
	SimConnection(ip_t ip, port_t port);
};

} // namespace hxcomm
