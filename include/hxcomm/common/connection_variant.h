#pragma once

#include "hxcomm/common/arqconnection.h"
#include "hxcomm/common/quiggeldy_client.h"
#include "hxcomm/common/simconnection.h"

#include <variant>

namespace hxcomm {

template <typename ConnectionParameter, typename RcfInterface>
using ConnectionVariantType = std::variant<
    hxcomm::ARQConnection<ConnectionParameter>,
    hxcomm::SimConnection<ConnectionParameter>,
    hxcomm::QuiggeldyClient<ConnectionParameter, RcfInterface>>;

} // namespace hxcomm
