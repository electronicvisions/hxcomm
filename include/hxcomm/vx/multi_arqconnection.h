#pragma once

#include "hxcomm/common/multiconnection.h"
#include "hxcomm/vx/arqconnection.h"

namespace hxcomm::vx {

using MultiARQConnection = hxcomm::MultiConnection<ARQConnection>;

} // namespace hxcomm::vx