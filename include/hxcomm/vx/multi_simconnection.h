#pragma once

#include "hxcomm/common/multiconnection.h"
#include "hxcomm/vx/simconnection.h"

namespace hxcomm::vx {

using MultiSimConnection = hxcomm::MultiConnection<SimConnection>;

} // namespace hxcomm::vx