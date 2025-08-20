#pragma once
#include "cereal/types/halco/common/geometry.h"
#include "cereal/types/halco/common/misc_types.h"
#include "cereal/types/hwdb/entries.h"
#include "hxcomm/common/hwdb_entry.h"
#include <cereal/cereal.hpp>

namespace cereal {

template <typename Archive>
void serialize(Archive&, hxcomm::SimulationEntry&)
{
}

template <typename Archive>
void serialize(Archive&, hxcomm::ZeroMockEntry&)
{
}

} // namespace cereal
