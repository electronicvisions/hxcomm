#pragma once

#include "pyhxcomm/vx/genpybind.h"

#include "hxcomm/vx/quiggeldy_utility.h"

namespace pyhxcomm { namespace vx GENPYBIND_TAG_HXCOMM_VX {

GENPYBIND_MANUAL({
	parent.def("get_unused_port", &::hxcomm::get_unused_port);
	parent.def("terminate", &::hxcomm::terminate);
	parent.def(
	    "launch_quiggeldy_locally_from_env", &::hxcomm::vx::launch_quiggeldy_locally_from_env);
	parent.def("unset_quiggeldy_env", &::hxcomm::vx::unset_quiggeldy_env);
})

} // namespace vx
} // namespace pyhxcomm
