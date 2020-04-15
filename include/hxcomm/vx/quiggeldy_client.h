#pragma once

// Needs to be included first because it sets up the RCF interface via macros
#include "hxcomm/vx/genpybind.h"
#include "hxcomm/vx/quiggeldy_rcf.h"

#include "hxcomm/common/quiggeldy_client.h"

#include <vector>

namespace hxcomm::vx GENPYBIND_TAG_HXCOMM_VX {

using QuiggeldyClient =
    hxcomm::QuiggeldyClient<hxcomm::vx::ConnectionParameter, hxcomm::vx::detail::rcf_client_type>;

GENPYBIND_MANUAL({
	using QuiggeldyClient = ::hxcomm::vx::QuiggeldyClient;

	parent->py::template class_<QuiggeldyClient>(parent, "QuiggeldyClient")
	    .def(parent->py::template init<>())
	    .def(parent->py::template init<hxcomm::ip_t, hxcomm::port_t>())
	    .def("supports", &QuiggeldyClient::supports)
	    .def_property("use_munge", &QuiggeldyClient::get_use_munge, &QuiggeldyClient::set_use_munge)
	    .def_property(
	        "connection_attempts_max", &QuiggeldyClient::get_connection_attempts_max,
	        &QuiggeldyClient::set_connection_attempts_max)
	    .def_property(
	        "connection_attempt_wait_after", &QuiggeldyClient::get_connection_attempt_wait_after,
	        &QuiggeldyClient::set_connection_attempt_wait_after);
})

} // namespace hxcomm::vx
