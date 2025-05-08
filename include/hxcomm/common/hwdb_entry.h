#pragma once
#include "hate/visibility.h"
#include "hwdb4cpp/hwdb4cpp.h"
#include <variant>

namespace hxcomm {

struct SimulationEntry
{};

struct ZeroMockEntry
{};

using HwdbEntry = std::
    variant<hwdb4cpp::HXCubeSetupEntry, hwdb4cpp::JboaSetupEntry, SimulationEntry, ZeroMockEntry>;

} // namespace hxcomm
