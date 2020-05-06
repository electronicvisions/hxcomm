#pragma once
#include "hxcomm/common/target_restriction.h"
#include "hxcomm/vx/genpybind.h"

namespace hxcomm::vx GENPYBIND_TAG_HXCOMM_VX {

typedef hxcomm::TargetRestriction TargetRestriction GENPYBIND(opaque(false));

using hxcomm::all_target_restrictions;

} // namespace hxcomm::vx
