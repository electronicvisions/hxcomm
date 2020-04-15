#pragma once

#ifdef QUIGGELDY_REQUEST_T
#warn "QUIGGELDY_REQUEST_T already defined "                                                       \
      "(are you building quiggeldy for several architectures?), undefining.."
#undef QUIGGELDY_REQUEST_T
#endif

#ifdef QUIGGELDY_RESPONSE_T
#warn "QUIGGELDY_RESPONSE_T already defined "                                                      \
      "(are you building quiggeldy for several architectures?), undefining.."
#undef QUIGGELDY_RESPONSE_T
#endif

#ifdef QUIGGELDY_RCF_INTERFACE
#warn "QUIGGELDY_RCF_INTERFACE already defined "                                                   \
      "(are you building quiggeldy for several architectures?), undefining.."
#undef QUIGGELDY_RCF_INTERFACE
#endif

#include "hxcomm/common/connection.h"
#include "hxcomm/vx/connection_parameter.h"

namespace hxcomm::vx {

using vx_message_types = MessageTypes<hxcomm::vx::ConnectionParameter>;

using quiggeldy_request_type = std::vector<vx_message_types::send_type>;
using quiggeldy_response_type = std::vector<vx_message_types::receive_type>;

class I_HXCommQuiggeldyVX;

} // namespace hxcomm::vx

#define QUIGGELDY_REQUEST_T hxcomm::vx::quiggeldy_request_type
#define QUIGGELDY_RESPONSE_T hxcomm::vx::quiggeldy_response_type
#define QUIGGELDY_RCF_INTERFACE hxcomm::vx::I_HXCommQuiggeldyVX

#include "hxcomm/common/quiggeldy_rcf.h"

#undef QUIGGELDY_REQUEST_T
#undef QUIGGELDY_RESPONSE_T
#undef QUIGGELDY_RCF_INTERFACE

namespace hxcomm::vx::detail {

#ifndef __GENPYBIND__
using rcf_client_type = hxcomm::RcfClient<hxcomm::vx::I_HXCommQuiggeldyVX>;
#else
struct rcf_client_type
{};
#endif

} // namespace hxcomm::vx::detail
