#include <chrono>
#include <sstream>
#include <thread>

#include "hxcomm/common/execute_messages.h"
#include "hxcomm/common/logger.h"
#include "hxcomm/common/quiggeldy_client.h"
#include "hxcomm/common/quiggeldy_common.h"

#include "RCF/RCF.hpp"

#include "logger.h"
#include "logging_ctrl.h"

#include "slurm/vision_defines.h"

namespace hxcomm {

using namespace std::literals::chrono_literals;

template <typename ConnectionParameter, typename RcfClient, template <typename> class Sequence>
QuiggeldyClient<ConnectionParameter, RcfClient, Sequence>::QuiggeldyClient() :
    QuiggeldyClient(get_connect_params_from_env())
{}

template <typename ConnectionParameter, typename RcfClient, template <typename> class Sequence>
QuiggeldyClient<ConnectionParameter, RcfClient, Sequence>::QuiggeldyClient(
    std::string ip, uint16_t port) :
    QuiggeldyClient(std::make_tuple(ip, port))
{}

template <typename ConnectionParameter, typename RcfClient, template <typename> class Sequence>
QuiggeldyClient<ConnectionParameter, RcfClient, Sequence>::QuiggeldyClient(
    QuiggeldyClient&&) noexcept = default;

template <typename ConnectionParameter, typename RcfClient, template <typename> class Sequence>
QuiggeldyClient<ConnectionParameter, RcfClient, Sequence>&
QuiggeldyClient<ConnectionParameter, RcfClient, Sequence>::operator=(QuiggeldyClient&&) noexcept =
    default;

template <typename ConnectionParameter, typename RcfClient, template <typename> class Sequence>
typename QuiggeldyClient<ConnectionParameter, RcfClient, Sequence>::connect_parameters_type
QuiggeldyClient<ConnectionParameter, RcfClient, Sequence>::get_connect_params_from_env()
{
	char const* env_ip = std::getenv(vision_quiggeldy_ip_env_name);
	if (env_ip == nullptr) {
		std::stringstream ss;
		ss << vision_quiggeldy_ip_env_name << " is not set and was not explictly provided.";
		auto const message = ss.str();

		HXCOMM_LOG_ERROR(m_logger, message);
		throw std::logic_error(message);
	}
	char const* env_port = std::getenv(vision_quiggeldy_port_env_name);
	if (env_port == nullptr) {
		std::stringstream ss;
		ss << vision_quiggeldy_port_env_name << " is not set and was not explictly provided.";
		auto const message = ss.str();

		HXCOMM_LOG_ERROR(m_logger, message);
		throw std::logic_error(message);
	}

	std::string ip(env_ip);
	uint16_t port(static_cast<uint16_t>(atoi(env_port)));

	if (port == 0) {
		auto message = "Invalid port provided.";

		HXCOMM_LOG_ERROR(m_logger, message);
		throw std::logic_error(message);
	}

	return std::make_tuple(ip, port);
}

template <typename ConnectionParameter, typename RcfClient, template <typename> class Sequence>
QuiggeldyClient<ConnectionParameter, RcfClient, Sequence>::QuiggeldyClient(
    typename QuiggeldyClient<ConnectionParameter, RcfClient, Sequence>::
        connect_parameters_type const& params) :
    m_connection_attempt_num_max(100),
    m_connection_attempt_wait_after(100ms),
    m_logger(log4cxx::Logger::getLogger("QuiggeldyClient")),
    m_use_munge(true)
{
	setup_client(std::get<0>(params), std::get<1>(params));
#ifdef USE_MUNGE_AUTH
	if (!is_munge_available()) {
		HXCOMM_LOG_WARN(
		    m_logger, "Munge socket does not appear to exist! Remote execution might fail!");
		m_use_munge = false;
	}
	m_munge_ctx = munge_ctx_setup();
#endif
}

template <typename ConnectionParameter, typename RcfClient, template <typename> class Sequence>
void QuiggeldyClient<ConnectionParameter, RcfClient, Sequence>::setup_client(
    std::string const& ip, uint16_t port)
{
	HXCOMM_LOG_DEBUG(m_logger, "Connecting to " << ip << ":" << port);

	RCF::init();
	m_client = std::make_unique<rcf_client_type>(RCF::TcpEndpoint(ip, port));

	m_client->getClientStub().getTransport().setMaxIncomingMessageLength(
	    quiggeldy_max_message_length);
	m_client->getClientStub().setRemoteCallTimeoutMs(
	    std::chrono::milliseconds(program_runtime_max).count());
}

template <typename ConnectionParameter, typename RcfClient, template <typename> class Sequence>
void QuiggeldyClient<ConnectionParameter, RcfClient, Sequence>::prepare_request()
{
#ifdef USE_MUNGE_AUTH
	if (m_use_munge) {
		char* cred;

		munge_err_t err = munge_encode(&cred, m_munge_ctx, NULL, 0);
		if (err != EMUNGE_SUCCESS) {
			HXCOMM_LOG_ERROR(
			    m_logger, "Could not encode credentials via munge. Is munge running? ERROR: "
			                  << munge_strerror(err));
		}

		m_client->getClientStub().setRequestUserData(std::string(cred));

		free(cred);
	}
#else
	if (false) {
	}
#endif
	else {
		m_client->getClientStub().setRequestUserData(
		    std::string(std::getenv("USER")) + "-without-authentication");
	}
}

template <typename ConnectionParameter, typename RcfClient, template <typename> class Sequence>
QuiggeldyClient<ConnectionParameter, RcfClient, Sequence>::~QuiggeldyClient()
{
	// client has to be destructed before we deinitialize!
	m_client.reset();
	RCF::deinit();
#ifdef USE_MUNGE_AUTH
	munge_ctx_destroy(m_munge_ctx);
#endif
}

template <typename ConnectionParameter, typename RcfClient, template <typename> class Sequence>
void QuiggeldyClient<ConnectionParameter, RcfClient, Sequence>::set_connection_attempts_max(
    size_t num)
{
	m_connection_attempt_num_max = num;
}

template <typename ConnectionParameter, typename RcfClient, template <typename> class Sequence>
size_t QuiggeldyClient<ConnectionParameter, RcfClient, Sequence>::get_connection_attempts_max()
{
	return m_connection_attempt_num_max;
}

template <typename ConnectionParameter, typename RcfClient, template <typename> class Sequence>
void QuiggeldyClient<ConnectionParameter, RcfClient, Sequence>::set_connection_attempt_wait_after(
    std::chrono::milliseconds ms)
{
	m_connection_attempt_wait_after = ms;
}

template <typename ConnectionParameter, typename RcfClient, template <typename> class Sequence>
std::chrono::milliseconds
QuiggeldyClient<ConnectionParameter, RcfClient, Sequence>::get_connection_attempt_wait_after()
{
	return m_connection_attempt_wait_after;
}

template <typename ConnectionParameter, typename RcfClient, template <typename> class Sequence>
void QuiggeldyClient<ConnectionParameter, RcfClient, Sequence>::set_use_munge(bool value)
{
	m_use_munge = value;
}

template <typename ConnectionParameter, typename RcfClient, template <typename> class Sequence>
bool QuiggeldyClient<ConnectionParameter, RcfClient, Sequence>::get_use_munge()
{
	return m_use_munge;
}

template <typename ConnectionParameter, typename RcfClient, template <typename> class Sequence>
typename QuiggeldyClient<ConnectionParameter, RcfClient, Sequence>::quiggeldy_response_type
QuiggeldyClient<ConnectionParameter, RcfClient, Sequence>::submit_blocking(
    quiggeldy_request_type const& request)
{
	prepare_request();

	quiggeldy_response_type response;

	for (size_t attempts_performed = 1; attempts_performed <= m_connection_attempt_num_max;
	     ++attempts_performed) {
		// build request and send it to server
		try {
			response = m_client->submit_work(request);
			break;
		} catch (const RCF::Exception& e) {
			if (e.getErrorId() != RCF::RcfError_ClientConnectFail ||
			    attempts_performed == m_connection_attempt_num_max) {
				// reraise if something unexpected happened or we reached the
				// maximum number of tries
				throw;
			}
		}

		if (m_logger->isEnabledFor(log4cxx::Level::getInfo())) {
			std::stringstream ss;
			ss << "Server not ready yet, waiting " << m_connection_attempt_wait_after.count()
			   << " ms.. [Attempt: " << attempts_performed << "/" << m_connection_attempt_num_max
			   << "]";

			// Give the user feedback once we wait a noticeable amount of time (1 second)
			if ((attempts_performed - 1) * m_connection_attempt_wait_after > 1s) {
				HXCOMM_LOG_INFO(m_logger, ss.str());
			} else {
				HXCOMM_LOG_DEBUG(m_logger, ss.str());
			}
		}
		std::this_thread::sleep_for(m_connection_attempt_wait_after);
	}

	return response;
}

template <typename ConnectionParameter, typename RcfClient, template <typename> class Sequence>
bool QuiggeldyClient<ConnectionParameter, RcfClient, Sequence>::supports(
    TargetRestriction const restriction) const
{
	return (restriction == TargetRestriction::hardware_non_interactive);
}

template <typename ConnectionParameter, typename RcfClient, template <typename> class Sequence>
class Stream<QuiggeldyClient<ConnectionParameter, RcfClient, Sequence> >
{
public:
	// Same as "regular" Stream API
	using connection_type = QuiggeldyClient<ConnectionParameter, RcfClient, Sequence>;

	using receive_message_type = typename connection_type::receive_message_type;
	using send_message_type = typename connection_type::send_message_type;

	Stream(connection_type& conn) : m_connection(conn){};
	Stream(Stream const&) = delete;
	Stream& operator=(Stream const&) = delete;

	// "Custom" function that only accepts full vectors of UTmessages
	typename connection_type::quiggeldy_response_type submit_blocking(
	    typename connection_type::quiggeldy_request_type const& req)
	{
		return get_connection().submit_blocking(req);
	};

protected:
	connection_type& get_connection()
	{
		return m_connection;
	}
	connection_type const& get_connection() const
	{
		return m_connection;
	}

	connection_type& m_connection;
};

namespace detail {

template <typename ConnectionParameter, typename RcfClient, template <typename> class Sequence>
struct ExecutorMessages<QuiggeldyClient<ConnectionParameter, RcfClient, Sequence>, Sequence>
{
	using connection_type = QuiggeldyClient<ConnectionParameter, RcfClient, Sequence>;
	using receive_message_type = typename connection_type::receive_message_type;
	using send_message_type = typename connection_type::send_message_type;

	using return_type = Sequence<receive_message_type>;
	using messages_type = Sequence<send_message_type>;

	return_type operator()(connection_type& conn, messages_type const& messages)
	{
		Stream<connection_type> stream(conn);

		return stream.submit_blocking(messages);
	}
};

} // namespace detail

} // namespace hxcomm
