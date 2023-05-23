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

#include "slurm/vision_defines.h"

namespace hxcomm {

using namespace std::literals::chrono_literals;

template <typename ConnectionParameter, typename RcfClient>
template <typename Submitter>
auto QuiggeldyConnection<ConnectionParameter, RcfClient>::submit(Submitter const& submitter)
{
	// ensure reinit script is present
	m_reinit_uploader->refresh();

	auto const cur_sequence_num = next_sequence_number();

	auto client = setup_client();
	size_t attempts_performed = 0;

	auto last_user_notification = std::chrono::system_clock::now();
	for (attempts_performed = 1; attempts_performed <= m_connection_attempt_num_max;
	     ++attempts_performed) {
		// build request and send it to server
		try {
			return submitter(client, cur_sequence_num);
		} catch (const RCF::Exception& e) {
			if (e.getErrorId() != RCF::RcfError_ClientConnectFail.getErrorId() ||
			    attempts_performed == m_connection_attempt_num_max) {
				// reraise if something unexpected happened or we reached the
				// maximum number of tries
				throw;
			}
		}
		using namespace std::chrono_literals;
		// Give the user feedback once per second in order to not spam the
		// terminal
		if ((std::chrono::system_clock::now() - last_user_notification) > 1s) {
			HXCOMM_LOG_INFO(
			    m_logger, "Server not ready yet, waiting "
			                  << m_connection_attempt_wait_after.count()
			                  << " ms in between attempts.. [Attempt: " << attempts_performed << "/"
			                  << m_connection_attempt_num_max << "]");
			last_user_notification = std::chrono::system_clock::now();
		}
		std::this_thread::sleep_for(m_connection_attempt_wait_after);
	}
	// NOTE: Should never be reached.
	HXCOMM_LOG_FATAL(m_logger, "Could not submit request.");
	throw std::runtime_error("Error submitting request.");
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
