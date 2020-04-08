#pragma once

#include "hxcomm/common/arqconnection.h"
#include "hxcomm/common/simconnection.h"

#include <variant>

namespace hxcomm {

/**
 * The actual connection types that differ in their backend implementation.
 */
template <typename ConnectionParameter>
using ConnectionVariant = std::
    variant<hxcomm::ARQConnection<ConnectionParameter>, hxcomm::SimConnection<ConnectionParameter>>;

} // namespace hxcomm
