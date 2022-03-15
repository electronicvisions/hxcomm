#pragma once

#include "pyhxcomm/vx/genpybind.h"

#include "hxcomm/vx/quiggeldy_utility.h"

namespace pyhxcomm::vx GENPYBIND_TAG_HXCOMM_VX {

GENPYBIND_MANUAL({
	parent.def("get_unused_port", &::hxcomm::get_unused_port);
	parent.def("terminate", &::hxcomm::terminate);
	parent.def(
	    "launch_quiggeldy_locally_from_env", &::hxcomm::vx::launch_quiggeldy_locally_from_env);
})

} // namespace pyhxcomm::vx
