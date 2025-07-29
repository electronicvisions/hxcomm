#pragma once

#include "hxcomm/common/multiconnection.h"
#include "hxcomm/vx/zeromockconnection.h"

namespace hxcomm::vx {

using MultiZeroMockConnection = hxcomm::MultiConnection<ZeroMockConnection>;

} // namespace hxcomm::vx