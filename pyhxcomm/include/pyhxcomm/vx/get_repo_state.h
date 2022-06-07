#pragma once
#include "hxcomm/common/get_repo_state.h"
#include "pyhxcomm/vx/genpybind.h"

namespace pyhxcomm::vx GENPYBIND_TAG_HXCOMM_VX {

GENPYBIND_MANUAL({ parent.def("get_repo_state", &hxcomm::get_repo_state); })

} // namespace pyhxcomm::vx
