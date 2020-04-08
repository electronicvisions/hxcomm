#pragma once


#include "hxcomm/common/connection_variant.h"

#include "pyhxcomm/common/managed_connection.h"

namespace pyhxcomm {

/**
 * Helper variant defining all types that functions using execute_messages in
 * upper layers should be specialized over.
 */
template <typename ConnectionParameter>
using ConnectionHandle =
    typename detail::add_handle<hxcomm::ConnectionVariant<ConnectionParameter>>::type;

} // namespace pyhxcomm
