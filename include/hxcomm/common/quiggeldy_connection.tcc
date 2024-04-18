#include <algorithm>
#include <chrono>
#include <memory>
#include <sstream>
#include <thread>

#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "RCF/RCF.hpp"
#include "hxcomm/common/execute_messages.h"
#include "hxcomm/common/logger.h"
#include "hxcomm/common/quiggeldy_common.h"
#include "hxcomm/common/quiggeldy_connection.h"
#include "rcf-extensions/retrying-client-invoke.h"

#include "slurm/vision_defines.h"

namespace hxcomm {

using namespace std::literals::chrono_literals;

template <typename ConnectionParameter, typename RcfClient>
template <typename Function, typename... Args>
auto QuiggeldyConnection<ConnectionParameter, RcfClient>::retrying_client_invoke(
    bool with_user_data, Function&& function, Args&&... args)
{
	return rcf_extensions::retrying_client_invoke(
	    [this, with_user_data]() { return setup_client(with_user_data); },
	    m_connection_attempt_num_max, m_connection_attempt_wait_after, function,
	    std::forward<Args>(args)...);
}

template <typename ConnectionParameter, typename RcfClient>
template <typename Function, typename... Args>
auto QuiggeldyConnection<ConnectionParameter, RcfClient>::retrying_client_invoke(
    bool with_user_data, Function&& function, Args&&... args) const
{
	return rcf_extensions::retrying_client_invoke(
	    [this, with_user_data]() { return setup_client(with_user_data); },
	    m_connection_attempt_num_max, m_connection_attempt_wait_after, function,
	    std::forward<Args>(args)...);
}

template <typename ConnectionParameter, typename RcfClient>
template <typename Submitter>
auto QuiggeldyConnection<ConnectionParameter, RcfClient>::submit(Submitter const& submitter)
{
	// ensure reinit script is present
	m_reinit_uploader->refresh();

	auto const cur_sequence_num = next_sequence_number();
	return retrying_client_invoke(true, submitter, cur_sequence_num);
}

namespace detail {

template <typename ConnectionParameters, typename RcfClient>
struct ExecutorMessages<hxcomm::QuiggeldyConnection<ConnectionParameters, RcfClient>>
{
	using connection_type = QuiggeldyConnection<ConnectionParameters, RcfClient>;
	using receive_message_type = typename connection_type::receive_message_type;
	using send_message_type = typename connection_type::send_message_type;

	using response_type = typename connection_type::interface_types::response_type;
	using request_type = typename connection_type::interface_types::request_type;

	response_type operator()(connection_type& conn, request_type const& messages)
	{
		StreamRC<connection_type> stream(conn);
		return stream.submit_blocking(messages);
	}
};

} // namespace detail

} // namespace hxcomm
