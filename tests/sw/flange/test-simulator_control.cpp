#include <gtest/gtest.h>
#include "flange/simulator_control.h"

using namespace flange;

TEST(SimulatorControl, General)
{
	EXPECT_NO_THROW(SimulatorControl());

	SimulatorControl ctrl;
	EXPECT_EQ(ctrl.get_runnable(), false);

	ctrl.set_runnable(true);
	EXPECT_EQ(ctrl.get_runnable(), true);

	EXPECT_NO_THROW(ctrl.issue_terminate());

	SimulatorEvent event;
	EXPECT_NO_THROW(ctrl.push_event(event));

	auto received = ctrl.received_data_available();
	// simulation not running -> no data to receive
	EXPECT_EQ(received, false);

	// simulation not running -> no data to receive
	EXPECT_THROW(ctrl.pop_front(), std::runtime_error);

	// simulation not running -> no data to receive
	EXPECT_THROW(ctrl.pop_events(), std::runtime_error);

	// simulation time is 0, since no simulation is running
	EXPECT_EQ(ctrl.current_clk(), 0);
}
