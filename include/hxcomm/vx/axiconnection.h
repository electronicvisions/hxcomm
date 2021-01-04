#pragma once

#include "hxcomm/common/axiconnection.h"
#include "hxcomm/vx/connection_parameter.h"

namespace hxcomm::vx {

using AXIConnection = hxcomm::AXIConnection<ConnectionParameter>;
using FrickelExtMem = hxcomm::FrickelExtMem;

} // namespace hxcomm::vx
