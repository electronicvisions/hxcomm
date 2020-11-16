#pragma once

#include "hxcomm/common/stream.h"

#include "hate/type_list.h"

#include <variant>

namespace hxcomm {

template <typename ConnectionVariant>
using ConnectionFullStreamInterfaceVariant = hate::type_list_to_t<
    std::variant,
    hate::filter_type_list_t<
        hxcomm::supports_full_stream_interface,
        hate::type_list_from_t<ConnectionVariant>>>;

} // namespace hxcomm
