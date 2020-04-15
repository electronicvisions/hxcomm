#pragma once

#include "hxcomm/vx/quiggeldy_rcf.h"

#include "hxcomm/common/detail/quiggeldy_server.h"

namespace hxcomm::vx::detail {

template <typename Connection>
using QuiggeldyServer =
    hxcomm::detail::QuiggeldyServer<Connection, hxcomm::vx::I_HXCommQuiggeldyVX>;

template <typename Connection>
using QuiggeldyWorker = hxcomm::detail::QuiggeldyWorker<Connection>;

} // namespace hxcomm::vx::detail
