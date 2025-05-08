#pragma once
#include "hxcomm/common/hwdb_entry.h"
#include "pyhxcomm/vx/genpybind.h"

namespace pyhxcomm {
namespace vx GENPYBIND_TAG_HXCOMM_VX {

typedef hxcomm::ZeroMockEntry ZeroMockEntry GENPYBIND(opaque(false));
typedef hxcomm::SimulationEntry SimulationEntry GENPYBIND(opaque(false));

} // namespace vx
} // namespace pyhxcomm
