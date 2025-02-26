#pragma once
#include "hxcomm/common/get_repo_state.h"
#include "pyhxcomm/vx/genpybind.h"

namespace pyhxcomm {
namespace vx GENPYBIND_TAG_HXCOMM_VX {

GENPYBIND_MANUAL({ parent.def("get_repo_state", &hxcomm::get_repo_state); })

} // namespace vx
} // namespace pyhxcomm
