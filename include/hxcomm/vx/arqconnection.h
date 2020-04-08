#pragma once

#include "hxcomm/common/arqconnection.h"
#include "hxcomm/common/connect_to_remote_parameter_defs.h"
#include "hxcomm/vx/connection_parameter.h"
#include "hxcomm/vx/genpybind.h"

namespace hxcomm::vx GENPYBIND_TAG_HXCOMM_VX {

using ARQConnection = hxcomm::ARQConnection<ConnectionParameter>;

GENPYBIND_MANUAL({
	using ARQConnection = ::hxcomm::vx::ARQConnection;
	parent->py::template class_<ARQConnection>(parent, "ARQConnection")
	    .def(parent->py::template init<>())
	    .def(parent->py::template init<hxcomm::ip_t>());
})

} // namespace hxcomm::vxGENPYBIND_TAG_HXCOMM_VX
