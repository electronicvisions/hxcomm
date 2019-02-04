#pragma once

#include <cstdint>
#include <deque>

#include <SF/deque.hpp>

#include <RCF/RCF.hpp>

#include "flange/simulator_event.h"

namespace flange {

RCF_BEGIN(I_SimulatorControl, "I_SimulatorControl")
RCF_METHOD_R0(bool, get_runnable)
RCF_METHOD_V1(void, set_runnable, bool)
RCF_METHOD_V0(void, issue_terminate)
RCF_METHOD_V1(void, push_event, SimulatorEvent const&)
RCF_METHOD_R0(bool, received_data_available)
RCF_METHOD_R0(SimulatorEvent, pop_front)
RCF_METHOD_R0(std::deque<SimulatorEvent>, pop_events)
RCF_METHOD_R0(SimulatorEvent::clk_t, current_clk)
RCF_END(I_SimulatorControl)

static constexpr uint16_t default_rcf_port = 50001;

} // namespace flange
