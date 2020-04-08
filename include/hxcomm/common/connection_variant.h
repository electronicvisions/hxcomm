#pragma once

#include "hxcomm/common/arqconnection.h"
#include "hxcomm/common/simconnection.h"

#include <variant>

namespace hxcomm {

template <typename ConnectionParameter>
using ConnectionVariantType = std::
    variant<hxcomm::ARQConnection<ConnectionParameter>, hxcomm::SimConnection<ConnectionParameter>>;

} // namespace hxcomm
