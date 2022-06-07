#include <algorithm>
#include <chrono>
#include <memory>
#include <sstream>
#include <thread>

#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "RCF/RCF.hpp"
#include "hate/timer.h"
#include "hxcomm/common/execute_messages.h"
#include "hxcomm/common/logger.h"
#include "hxcomm/common/quiggeldy_common.h"
#include "hxcomm/common/quiggeldy_connection.h"

#include "slurm/vision_defines.h"

namespace hxcomm {

using namespace std::literals::chrono_literals;

template <typename ConnectionParameter, typename RcfClient>
QuiggeldyConnection<ConnectionParameter, RcfClient>::QuiggeldyConnection() :
    QuiggeldyConnection(get_connect_params_from_env())
{}

template <typename ConnectionParameter, typename RcfClient>
QuiggeldyConnection<ConnectionParameter, RcfClient>::QuiggeldyConnection(
    std::string ip, uint16_t port) :
    QuiggeldyConnection(std::make_tuple(ip, port))
{}

template <typename ConnectionParameter, typename RcfClient>
typename QuiggeldyConnection<ConnectionParameter, RcfClient>::connect_parameters_type
QuiggeldyConnection<ConnectionParameter, RcfClient>::get_connect_params_from_env()
{
	// This function is called when QuiggeldyConnection is not yet initialized
	auto const logger =
	    log4cxx::Logger::getLogger("QuiggeldyConnection.get_connect_params_from_env");

	char const* env_ip = std::getenv(vision_quiggeldy_ip_env_name);
	if (env_ip == nullptr) {
		std::stringstream ss;
		ss << vision_quiggeldy_ip_env_name << " is not set and was not explictly provided.";
		auto const message = ss.str();

		HXCOMM_LOG_ERROR(logger, message);
		throw std::logic_error(message);
	}
	char const* env_port = std::getenv(vision_quiggeldy_port_env_name);
	if (env_port == nullptr) {
		std::stringstream ss;
		ss << vision_quiggeldy_port_env_name << " is not set and was not explictly provided.";
		auto const message = ss.str();

		HXCOMM_LOG_ERROR(logger, message);
		throw std::logic_error(message);
	}

	std::string ip{env_ip};
	uint16_t port{static_cast<uint16_t>(atoi(env_port))};

	if (port == 0) {
		auto message = "Invalid port provided.";

		HXCOMM_LOG_ERROR(logger, message);
		throw std::logic_error(message);
	}

	HXCOMM_LOG_TRACE(
	    logger, "Extracted quiggeldy connect info from environment: " << ip << ":" << port);
	return std::make_tuple(std::move(ip), std::move(port));
}

template <typename ConnectionParameter, typename RcfClient>
std::lock_guard<std::mutex> QuiggeldyConnection<ConnectionParameter, RcfClient>::lock_time_info()
    const
{
	return std::lock_guard{m_mutex_time_info};
}

template <typename ConnectionParameter, typename RcfClient>
QuiggeldyConnection<ConnectionParameter, RcfClient>::QuiggeldyConnection(
    typename QuiggeldyConnection<ConnectionParameter, RcfClient>::connect_parameters_type const&
        params) :
    m_connect_parameters{params},
    m_connection_attempt_num_max(100),
    m_connection_attempt_wait_after(100ms),
    m_logger(log4cxx::Logger::getLogger("QuiggeldyConnection")),
    m_reinit_uploader{new reinit_uploader_type{
        get_create_client_function(), &rcf_client_type::reinit_notify,
        &rcf_client_type::reinit_pending, &rcf_client_type::reinit_upload}},
    m_sequence_num(0),
    m_reinit_stack{new reinit_stack_type{}}
{
#ifdef USE_MUNGE_AUTH
	constexpr bool has_munge_support = true;
#else
	constexpr bool has_munge_support = false;
#endif
	m_session_uuid = boost::uuids::random_generator()();
	try {
		// Check if remote site has munge enabled.
		m_use_munge = setup_client(false)->get_use_munge();
	} catch (const RCF::Exception& e) {
		HXCOMM_LOG_ERROR(m_logger, "Could not request munge status from remote site: " << e.what());
		m_use_munge = false;
	}

	if (m_use_munge && !has_munge_support) {
		std::string const msg{"Quiggeldy server expects client to use munge, but munge support was "
		                      "not configured. Please re-configure with --with-munge."};
		HXCOMM_LOG_ERROR(m_logger, msg);
		throw std::runtime_error(msg);
	}

#ifdef USE_MUNGE_AUTH
	if (m_use_munge && !is_munge_available()) {
		std::string const msg{
		    "Quiggeldy server expects client to use munge, but munge socket does not exist!"};
		HXCOMM_LOG_ERROR(m_logger, msg);
		throw std::runtime_error(msg);
	}
#endif
}

template <typename ConnectionParameter, typename RcfClient>
QuiggeldyConnection<ConnectionParameter, RcfClient>::QuiggeldyConnection(
    QuiggeldyConnection&& other) :
    m_connect_parameters(std::move(other.m_connect_parameters)),
    m_connection_attempt_num_max(std::move(other.m_connection_attempt_num_max)),
    m_connection_attempt_wait_after(std::move(other.m_connection_attempt_wait_after)),
    m_logger(log4cxx::Logger::getLogger("QuiggeldyConnection")),
    m_use_munge(std::move(other.m_use_munge)),
    m_session_uuid(std::move(other.m_session_uuid)),
    m_reinit_uploader(std::move(other.m_reinit_uploader)),
    m_sequence_num(std::move(other.m_sequence_num)),
    m_reinit_stack(std::move(other.m_reinit_stack)),
    m_custom_user(std::move(other.m_custom_user))
{
	RCF::init();
	HXCOMM_LOG_TRACE(m_logger, "Moving QuiggeldyConnection!");
	auto const lk = lock_time_info();
	m_time_info = std::move(other.m_time_info);
	// Update lambda to contain correct this pointer
	m_reinit_uploader->update_function_create_client(get_create_client_function());
}

template <typename ConnectionParameter, typename RcfClient>
QuiggeldyConnection<ConnectionParameter, RcfClient>::~QuiggeldyConnection()
{
	if (m_reinit_uploader.use_count() > 1) {
		RCF_LOG_ERROR(
		    m_logger, "ReinitUploader is still used by " << m_reinit_uploader.use_count()
		                                                 << ", but expected 1.");
	}
	m_reinit_uploader.reset();

	if (m_reinit_stack.use_count() > 1) {
		RCF_LOG_ERROR(
		    m_logger,
		    "ReinitStack is still used by " << m_reinit_stack.use_count() << ", but expected 1.");
	}
	m_reinit_stack.reset();
	RCF_LOG_TRACE(m_logger, "RCF::getInitRefCount(): " << RCF::getInitRefCount());
}

template <typename ConnectionParameter, typename RcfClient>
QuiggeldyConnection<ConnectionParameter, RcfClient>&
QuiggeldyConnection<ConnectionParameter, RcfClient>::operator=(
    QuiggeldyConnection<ConnectionParameter, RcfClient>&& other)
{
	if (&other != this) {
		m_connect_parameters = std::move(other.m_connect_parameters);
		m_connection_attempt_num_max = std::move(other.m_connection_attempt_num_max);
		m_connection_attempt_wait_after = std::move(other.m_connection_attempt_wait_after);
		m_logger = log4cxx::Logger::getLogger("QuiggeldyConnection");
		m_use_munge = std::move(other.m_use_munge);
		m_session_uuid = std::move(other.m_session_uuid);
		m_reinit_uploader = std::move(other.m_reinit_uploader);
		m_sequence_num = std::move(other.m_sequence_num);
		m_reinit_stack = std::move(other.m_reinit_stack);
		m_custom_user = std::move(other.m_custom_user);
		{
			auto const lk = lock_time_info();
			m_time_info = std::move(other.m_time_info);
		}
		// Update lambda to contain correct this pointer
		m_reinit_uploader->update_function_create_client(get_create_client_function());
	}
	return *this;
}

template <typename ConnectionParameter, typename RcfClient>
std::unique_ptr<RcfClient> QuiggeldyConnection<ConnectionParameter, RcfClient>::setup_client(
    bool with_user_data) const
{
	auto const ip = std::get<0>(m_connect_parameters);
	auto const port = std::get<1>(m_connect_parameters);

	HXCOMM_LOG_DEBUG(m_logger, "Connecting to " << ip << ":" << port);
	auto client = std::make_unique<rcf_client_type>(RCF::TcpEndpoint(ip, port));

	client->getClientStub().getTransport().setMaxIncomingMessageLength(
	    quiggeldy_max_message_length);
	client->getClientStub().setRemoteCallTimeoutMs(
	    std::chrono::milliseconds(program_runtime_max).count());

	if (with_user_data) {
		set_user_data(client);
	}

	// ensure client is connected
	if (!client->getClientStub().isConnected()) {
		[[maybe_unused]] hate::Timer timer;
		client->getClientStub().connect();
		HXCOMM_LOG_DEBUG(m_logger, "setup_client(): Connecting client took " << timer.print());
	}

	return client;
}

template <typename ConnectionParameter, typename RcfClient>
void QuiggeldyConnection<ConnectionParameter, RcfClient>::set_user_data(
    std::unique_ptr<rcf_client_type>& client) const
{
#ifdef USE_MUNGE_AUTH
	if (m_use_munge) {
		HXCOMM_LOG_DEBUG(m_logger, "Encoding session id via munge.");
		char* cred;
		auto munge_ctx = munge_ctx_setup();

		std::stringstream ss;
		ss << m_session_uuid;

		auto session_id = ss.str();
		char const* session_id_c_str = session_id.c_str();

		// string::length returns the number of characters in the string, but the c_str has an
		// additional terminating \0-byte
		munge_err_t err = munge_encode(
		    &cred, munge_ctx, static_cast<void const*>(session_id_c_str), session_id.length() + 1);
		if (err != EMUNGE_SUCCESS) {
			HXCOMM_LOG_ERROR(
			    m_logger, "Could not encode credentials via munge. Is munge running? ERROR: "
			                  << munge_strerror(err));
		}

		client->getClientStub().setRequestUserData(std::string(cred));

		munge_ctx_destroy(munge_ctx);
		free(cred);
	} else
#endif
	{
		std::stringstream ss;
		// In non-munge case, check if the user provides a different username by which to
		// authenticate.
		if (m_custom_user) {
			HXCOMM_LOG_DEBUG(m_logger, "Setting non-munge custom username: " << *m_custom_user);
			ss << *m_custom_user;
		} else {
			auto const env_user = std::getenv("USER");
			HXCOMM_LOG_DEBUG(m_logger, "Setting non-munge username: " << env_user);
			ss << env_user;
		}
		ss << ":" << m_session_uuid;
		client->getClientStub().setRequestUserData(ss.str());
	}
}

template <typename ConnectionParameter, typename RcfClient>
std::weak_ptr<typename QuiggeldyConnection<ConnectionParameter, RcfClient>::reinit_uploader_type>
QuiggeldyConnection<ConnectionParameter, RcfClient>::get_reinit_upload() const
{
	return m_reinit_uploader;
}

template <typename ConnectionParameter, typename RcfClient>
std::weak_ptr<typename QuiggeldyConnection<ConnectionParameter, RcfClient>::reinit_stack_type>
QuiggeldyConnection<ConnectionParameter, RcfClient>::get_reinit_stack() const
{
	return m_reinit_stack;
}

template <typename ConnectionParameter, typename RcfClient>
void QuiggeldyConnection<ConnectionParameter, RcfClient>::reinit_enforce()
{
	setup_client()->reinit_enforce();
}

template <typename ConnectionParameter, typename RcfClient>
void QuiggeldyConnection<ConnectionParameter, RcfClient>::set_connection_attempts_max(size_t num)
{
	m_connection_attempt_num_max = num;
}

template <typename ConnectionParameter, typename RcfClient>
size_t QuiggeldyConnection<ConnectionParameter, RcfClient>::get_connection_attempts_max() const
{
	return m_connection_attempt_num_max;
}

template <typename ConnectionParameter, typename RcfClient>
void QuiggeldyConnection<ConnectionParameter, RcfClient>::set_connection_attempt_wait_after(
    std::chrono::milliseconds ms)
{
	m_connection_attempt_wait_after = ms;
}

template <typename ConnectionParameter, typename RcfClient>
std::chrono::milliseconds
QuiggeldyConnection<ConnectionParameter, RcfClient>::get_connection_attempt_wait_after() const
{
	return m_connection_attempt_wait_after;
}

template <typename ConnectionParameter, typename RcfClient>
void QuiggeldyConnection<ConnectionParameter, RcfClient>::set_use_munge(bool value)
{
	m_use_munge = value;
}

template <typename ConnectionParameter, typename RcfClient>
bool QuiggeldyConnection<ConnectionParameter, RcfClient>::get_use_munge() const
{
	return m_use_munge;
}

template <typename ConnectionParameter, typename RcfClient>
rcf_extensions::SequenceNumber
QuiggeldyConnection<ConnectionParameter, RcfClient>::next_sequence_number()
{
	std::lock_guard<std::mutex> lk(m_mutex_sequence_num);
	return m_sequence_num++;
}

template <typename ConnectionParameter, typename RcfClient>
typename QuiggeldyConnection<ConnectionParameter, RcfClient>::interface_types::response_type
QuiggeldyConnection<ConnectionParameter, RcfClient>::submit_blocking(
    typename interface_types::request_type const& request)
{
	return submit([&request, this](auto& client, auto& sequence_num) {
		typename interface_types::response_type response =
		    client->submit_work(request, sequence_num);
		accumulate_time_info(response.second);
		return response;
	});
}

template <typename ConnectionParameter, typename RcfClient>
typename QuiggeldyConnection<ConnectionParameter, RcfClient>::future_type
QuiggeldyConnection<ConnectionParameter, RcfClient>::submit_async(
    typename interface_types::request_type const& request)
{
	return submit([&request, this](auto& client, auto const& sequence_num) {
		client->getClientStub().connect();
		// Technically, we could probably avoid the unique_ptr for
		// RCF::Future, however, it would create an asymmetry between the
		// client and future instance.
		auto rcf_future_ptr =
		    std::make_unique<RCF::Future<typename interface_types::response_type>>();

		*rcf_future_ptr = client->submit_work(
		    RCF::AsyncTwoway([this, rcf_future(*rcf_future_ptr)]() mutable {
			    typename interface_types::response_type& response = rcf_future;
			    // track time
			    accumulate_time_info(response.second);
		    }),
		    request, sequence_num);
		return future_type{std::move(client), std::move(rcf_future_ptr)};
	});
}

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

template <typename ConnectionParameter, typename RcfClient>
bool QuiggeldyConnection<ConnectionParameter, RcfClient>::is_out_of_order() const
{
	return m_sequence_num.is_out_of_order();
}

template <typename ConnectionParameter, typename RcfClient>
ConnectionTimeInfo QuiggeldyConnection<ConnectionParameter, RcfClient>::get_time_info() const
{
	auto const lk = lock_time_info();
	return m_time_info;
}

template <typename ConnectionParameter, typename RcfClient>
void QuiggeldyConnection<ConnectionParameter, RcfClient>::accumulate_time_info(
    ConnectionTimeInfo const& delta)
{
	auto const lk = lock_time_info();
	m_time_info += delta;
}

template <typename ConnectionParameter, typename RcfClient>
std::string QuiggeldyConnection<ConnectionParameter, RcfClient>::get_unique_identifier(
    std::optional<std::string> hwdb_path) const
{
	auto client = setup_client();
	return client->get_unique_identifier(hwdb_path);
}

template <typename ConnectionParameter, typename RcfClient>
std::string QuiggeldyConnection<ConnectionParameter, RcfClient>::get_bitfile_info() const
{
	auto client = setup_client();
	return client->get_bitfile_info();
}

template <typename ConnectionParameter, typename RcfClient>
std::string QuiggeldyConnection<ConnectionParameter, RcfClient>::get_version_string() const
{
	auto client = setup_client();
	return client->get_version_string();
}

template <typename ConnectionParameter, typename RcfClient>
void QuiggeldyConnection<ConnectionParameter, RcfClient>::set_out_of_order()
{
	m_sequence_num = rcf_extensions::SequenceNumber::out_of_order();
}

template <typename ConnectionParameter, typename RcfClient>
typename QuiggeldyConnection<ConnectionParameter, RcfClient>::f_create_client_shared_ptr_t
QuiggeldyConnection<ConnectionParameter, RcfClient>::get_create_client_function()
{
	return [this] {
		HXCOMM_LOG_DEBUG(m_logger, "Creating client in uploader.");
		std::shared_ptr<rcf_client_type> client{setup_client()};
		client->getClientStub().setRemoteCallTimeoutMs(24 * 60 * 60 * 7); // have a week to timeout
		return client;
	};
}

template <typename ConnectionParameter, typename RcfClient>
void QuiggeldyConnection<ConnectionParameter, RcfClient>::set_custom_username(
    std::string custom_user)
{
	// We use colon as separator between user and session information -> replace it.
	std::replace(custom_user.begin(), custom_user.end(), ':', '_');
	m_custom_user = std::move(custom_user);
	HXCOMM_LOG_DEBUG(m_logger, "Set custom user name to: " << *m_custom_user);
}

template <typename ConnectionParameter, typename RcfClient>
std::optional<std::string>
QuiggeldyConnection<ConnectionParameter, RcfClient>::get_custom_username()
{
	return m_custom_user;
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
