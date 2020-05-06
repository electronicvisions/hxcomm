#pragma once
#include "hxcomm/common/connect_to_remote_parameter_defs.h"
#include "hxcomm/common/simconnection.h"
#include "hxcomm/vx/connection_parameter.h"
#include "hxcomm/vx/genpybind.h"
#include "hxcomm/vx/target_restriction.h"

namespace hxcomm::vx GENPYBIND_TAG_HXCOMM_VX {

using SimConnection = hxcomm::SimConnection<ConnectionParameter>;

GENPYBIND_MANUAL({
	using SimConnection = ::hxcomm::vx::SimConnection;
	parent->py::template class_<SimConnection>(parent, "SimConnection")
	    .def(parent->py::template init<>())
	    .def(parent->py::template init<bool>())
	    .def(parent->py::template init<hxcomm::ip_t, hxcomm::port_t>())
	    .def(parent->py::template init<hxcomm::ip_t, hxcomm::port_t, bool>())
	    .def("supports", &::hxcomm::vx::SimConnection::supports);
})

} // namespace hxcomm::vx
