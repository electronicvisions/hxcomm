#pragma once

#include "hxcomm/vx/arqconnection.h"
#include "hxcomm/vx/simconnection.h"

#include <variant>

namespace hxcomm::vx {

using ConnectionVariant = std::variant<hxcomm::vx::ARQConnection, hxcomm::vx::SimConnection>;

} // namespace hxcomm::vx
