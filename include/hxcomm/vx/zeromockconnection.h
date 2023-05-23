#pragma once
#include "hate/visibility.h"
#include "hxcomm/common/zeromockconnection.h"
#include "hxcomm/vx/connection_parameter.h"


namespace hxcomm {

namespace detail {

template <>
struct ZeroMockProcessMessage<hxcomm::vx::ConnectionParameter>
{
	HXCOMM_EXPOSE_MESSAGE_TYPES(hxcomm::vx::ConnectionParameter)
	typedef std::vector<receive_message_type> receive_queue_type;

	ZeroMockProcessMessage(receive_queue_type& receive_queue, bool& halt) SYMBOL_VISIBLE;

	void operator()(send_message_type const& message) SYMBOL_VISIBLE;

private:
	receive_queue_type& m_receive_queue;
	bool& m_halt;
};

} // namespace detail

namespace vx {

using ZeroMockConnection = hxcomm::ZeroMockConnection<ConnectionParameter>;

} // namespace vx

extern template class SYMBOL_VISIBLE ZeroMockConnection<hxcomm::vx::ConnectionParameter>;

} // namespace hxcomm
