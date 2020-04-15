#pragma once

#include "hxcomm/common/quigggeldy_utility.h"
#include "hxcomm/vx/genpybind.h"


namespace hxcomm::vx GENPYBIND_TAG_HXCOMM_VX {

/**
 * Launch a local quiggeldy without slurm allocation and debug output based on
 * current env settings.
 *
 * @return Process ID of launched quiggeldy process and the port it is listening on.
 */
inline std::tuple<pid_t, hxcomm::port_t> launch_quiggely_locally_from_env();

} // namespace hxcomm::vx::quiggeldyGENPYBIND_TAG_HXCOMM_VX

GENPYBIND_MANUAL({
    parent.def("get_unused_port", &::hxcomm::get_unused_port);
    parent.def("terminate", &::hxcomm::terminate);
})

#ifndef __GENPYBIND__
#include "hxcomm/vx/quiggeldy_utility.tcc"
#endif
