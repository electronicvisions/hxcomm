#include "hxcomm/common/quiggeldy_worker.h"

#include "hxcomm/common/execute_messages.h"
#include "hxcomm/common/get_repo_state.h"
#include "hxcomm/common/logger.h"
#include "hxcomm/common/quiggeldy_common.h"
#include "hxcomm/common/quiggeldy_utility.h"

#include "hate/memory.h"

#include "slurm/vision_defines.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <unistd.h>
#include <unordered_map> // needed for std::hash<std::string>

#include <sys/wait.h>

namespace hxcomm {

using namespace std::literals::chrono_literals;

template <typename Connection>
template <typename... Args>
QuiggeldyWorker<Connection>::QuiggeldyWorker(
    std::optional<std::string> public_key,
    std::optional<std::string> encryption_method,
    std::optional<std::chrono::seconds> token_expiration_grace_time,
    Args&&... args) :
    m_public_key(public_key),
    m_token_encryption(encryption_method),
    m_connection_init(std::forward<Args>(args)...),
    m_has_slurm_allocation(false),
    m_mock_mode(false),
    m_logger(log4cxx::Logger::getLogger("hxcomm.QuiggeldyWorker")),
    m_use_munge(true),
    m_max_num_connection_attempts{10},
    m_delay_after_connection_attempt{1s}
{
	m_token_expiration_grace_time = token_expiration_grace_time.value_or(std::chrono::seconds{0});

	if (m_logger->isEnabledFor(log4cxx::Level::getTrace())) {
		std::stringstream ss;
		ss << "Passing " << (sizeof...(Args)) << " arguments to connection:";
		((ss << " " << args), ...);
		HXCOMM_LOG_TRACE(m_logger, ss.str());
	}
	char const* env_partition = std::getenv(vision_quiggeldy_partition_env_name);
	if (env_partition == nullptr) {
		m_slurm_partition = default_slurm_partition;
	} else {
		m_slurm_partition = env_partition;
	}
	{
		// set dummy slurm license
		std::stringstream ss;
		ss << "quiggeldy";
		auto paste = [&ss](auto&& arg) { ss << "_" << arg; };
		std::apply([&ss, &paste](auto&&... arg) { (..., paste(arg)); }, m_connection_init);
		m_slurm_license = ss.str();
	}
}

template <typename Connection>
QuiggeldyWorker<Connection>::~QuiggeldyWorker()
{
	HXCOMM_LOG_TRACE(m_logger, "Shutting down..");
	teardown();
	HXCOMM_LOG_TRACE(m_logger, "Shut down.");
}

template <typename Connection>
void QuiggeldyWorker<Connection>::slurm_allocation_acquire()
{
	// prevent error if we already have slurm allocation
	if (has_slurm_allocation()) {
		return;
	}
	HXCOMM_LOG_DEBUG(m_logger, "Getting slurm allocation for " << get_slurm_jobname() << ".");
	exec_slurm_binary(
	    "salloc", "salloc", "-p", m_slurm_partition.c_str(), "--no-shell", "--license",
	    get_slurm_license().c_str(), "--mem", "0M", "-J", get_slurm_jobname().c_str());

	m_has_slurm_allocation = true;
	HXCOMM_LOG_DEBUG(
	    m_logger,
	    "Slurm allocation for " << get_slurm_jobname().c_str() << " successfully acquired.");
}

template <typename Connection>
void QuiggeldyWorker<Connection>::setup()
{
	if (!m_mock_mode) {
		if (m_allocate_license) {
			slurm_allocation_acquire();
		}
		setup_connection();
	} else {
		HXCOMM_LOG_DEBUG(m_logger, "Operating in mock-mode - no connection allocated.");
	}
	HXCOMM_LOG_TRACE(m_logger, "setup() completed!");
}

template <typename Connection>
void QuiggeldyWorker<Connection>::set_user_token(
    std::string session_id, std::string const& user_token)
{
	HXCOMM_LOG_DEBUG(m_logger, "Setting user token.");

	// If no public key is provided to the server no token verification is performed and no tokens
	// need to be set.
	if (!m_public_key) {
		return;
	}

	std::map<std::string, std::string> claims;
	std::stringstream ss;

	if (!m_token_encryption) {
		throw std::runtime_error(
		    "No encryption method for user token specified, but trying to decode.");
	}

	try {
		verify_jwt(
		    user_token, *m_public_key, m_token_expiration_grace_time, std::make_optional(claims),
		    *m_token_encryption);
		m_session_tokens[session_id] = std::make_optional<std::string>(user_token);
		ss << "User token is valid.";
	} catch (std::logic_error const& e) {
		m_session_tokens[session_id] = std::nullopt;
		ss << "User token is invalid.";
		ss << "(" << e.what() << ")";
	}

	HXCOMM_LOG_INFO(m_logger, ss.str());
}

template <typename Connection>
void QuiggeldyWorker<Connection>::setup_connection()
{
	HXCOMM_LOG_TRACE(m_logger, "Setting up local connection.");
	// Release old connection first because otherwise we might block ourselves.
	m_connection.reset();
	// TODO: have the experiment control timeout (e.g. when the board is unresponsive)
	std::size_t num_attempts = 0;
	while (true) {
		try {
			auto new_connection =
			    hate::memory::make_unique_from_tuple<Connection>(m_connection_init);
			m_connection.swap(new_connection);
			break;
		} catch (std::exception& e) {
			if (num_attempts < m_max_num_connection_attempts) {
				++num_attempts;
				HXCOMM_LOG_WARN(
				    m_logger, "Could not establish connection: "
				                  << e.what() << " Waiting "
				                  << m_delay_after_connection_attempt.count() << "ms.. [Attempt: "
				                  << num_attempts << "/" << m_max_num_connection_attempts << "]");
				std::this_thread::sleep_for(m_delay_after_connection_attempt);
				continue;
			} else {
				HXCOMM_LOG_ERROR(m_logger, "Could not establish connection: " << e.what());
				throw e;
			}
		}
	}
}

template <typename Connection>
bool QuiggeldyWorker<Connection>::check_for_timeout(
    QuiggeldyWorker<Connection>::response_type const& response)
{
	HXCOMM_LOG_DEBUG(m_logger, "Checking for timeout.");

	bool timeout = false;

	// if any msg is a receive timeout trigger a reconnection
	if (std::any_of(response.cbegin(), response.cend(), [this](auto const& m) {
		    return std::visit(
		        [this](auto const& mm) {
			        return std::is_same_v<
			            typename std::remove_cvref_t<decltype(mm)>::instruction_type,
			            typename Connection::message_types::connection_parameter_type::
			                ReceiveTimeout>;
		        },
		        m);
	    })) {
		timeout = true;
	}
	HXCOMM_LOG_DEBUG(m_logger, "Checked for timeout.");
	return timeout;
}

template <typename Connection>
void QuiggeldyWorker<Connection>::teardown()
{
	HXCOMM_LOG_DEBUG(m_logger, "teardown() started..");
	if (!m_mock_mode) {
		m_connection.reset();
		if (m_allocate_license) {
			slurm_allocation_release();
		}
	}
	HXCOMM_LOG_DEBUG(m_logger, "teardown() completed!");
}

template <typename Connection>
template <typename... Args>
void QuiggeldyWorker<Connection>::exec_slurm_binary(char const* binary_name, Args&&... args)
{
	static_assert(
	    detail::all_char_const_ptr_v<Args...>,
	    "All arguments need to be char pointers to be given to the execlp-call!");

	int status;
	pid_t pid = fork();

	if (pid == 0) {
		if (m_logger->isEnabledFor(log4cxx::Level::getDebug())) {
			std::stringstream message;
			message << "Executing: ";
			((message << ' ' << std::forward<Args>(args)), ...);

			HXCOMM_LOG_DEBUG(m_logger, message.str());
		}
		execlp(binary_name, binary_name, std::forward<Args>(args)..., NULL);
		exit(0);
	} else {
		waitpid(pid, &status, 0); // wait for the child to change state (i.e., exit)
	}

	if (!WIFEXITED(status) || !(WEXITSTATUS(status) == 0)) {
		std::stringstream message;
		message << "Error executing: ";
		((message << ' ' << std::forward<Args>(args)), ...);
		HXCOMM_LOG_ERROR(m_logger, message.str());
		throw std::logic_error(message.str());
	}
}

template <typename Connection>
void QuiggeldyWorker<Connection>::slurm_allocation_release()
{
	// prevent error if we do not have any slurm allocation
	if (!has_slurm_allocation()) {
		return;
	}
	HXCOMM_LOG_DEBUG(
	    m_logger, "Freeing slurm allocation for " << get_slurm_jobname().c_str() << ".");

	exec_slurm_binary("scancel", "-n", get_slurm_jobname().c_str());
	m_has_slurm_allocation = false;

	HXCOMM_LOG_DEBUG(
	    m_logger, "Slurm allocation for " << get_slurm_jobname().c_str() << " successfully freed.");
}

template <typename Connection>
bool QuiggeldyWorker<Connection>::has_slurm_allocation()
{
	return m_has_slurm_allocation;
}

template <typename Connection>
typename QuiggeldyWorker<Connection>::return_type QuiggeldyWorker<Connection>::work(
    request_type const& req, boost::uuids::uuid const& session_id)
{
	if (m_sessions_with_failed_reinit.contains(session_id)) {
		// session-user might try again
		m_sessions_with_failed_reinit.erase(session_id);
		throw std::runtime_error("User session reinit failed.");
	}

	if (m_sessions_with_failed_reinit_schedule_out.contains(session_id)) {
		throw std::runtime_error(
		    "User session reinit schedule out failed, no further executions using this session id "
		    "possible. The current hardware state of this session could not be determined after "
		    "the last execution and can not be recovered. Please establish a new connection.");
	}

	if (m_mock_mode) {
		HXCOMM_LOG_DEBUG(m_logger, "Running mock-experiment!");
		return return_type{};
	}

	HXCOMM_LOG_TRACE(m_logger, "Executing program!");
	try {
		auto retval = execute_messages(*m_connection, req);
		if (check_for_timeout(std::get<0>(retval))) {
			HXCOMM_LOG_WARN(
			    m_logger,
			    "Encountered timeout notifications in response stream -> resetting connection.");
			setup_connection();
		}
		return retval;
	} catch (const std::exception& e) {
		HXCOMM_LOG_ERROR(m_logger, "Error during word execution: " << e.what());
		throw;
	}
}

template <typename Connection>
void QuiggeldyWorker<Connection>::perform_reinit(
    reinit_type& reinit, boost::uuids::uuid const& session_id, bool force)
{
	if (m_mock_mode) {
		HXCOMM_LOG_DEBUG(m_logger, "Running mock-reinit!");
		return;
	}

	HXCOMM_LOG_TRACE(m_logger, "Performing reinit!");

	try {
		for (auto& entry : reinit) {
			if (entry.reinit_pending || force) {
				execute_messages(*m_connection, entry.request);
				entry.reinit_pending = false;
			}
		}
	} catch (const std::exception& e) {
		HXCOMM_LOG_ERROR(m_logger, "Error during word execution: " << e.what());
		m_sessions_with_failed_reinit.insert(session_id);
		teardown();
	}
}

template <typename Connection>
void QuiggeldyWorker<Connection>::perform_reinit_snapshot(
    reinit_type& reinit, boost::uuids::uuid const& session_id)
{
	if (m_mock_mode) {
		HXCOMM_LOG_DEBUG(m_logger, "Running mock-reinit-snapshot!");
		return;
	}

	HXCOMM_LOG_TRACE(m_logger, "Performing reinit snapshot!");

	try {
		for (auto& entry : reinit) {
			if (entry.snapshot) {
				auto const [response, _] = execute_messages(*m_connection, *(entry.snapshot));
				entry.request = typename std::decay_t<decltype(entry)>::transform_type{}(
				    response, *(entry.snapshot));
			}
		}
	} catch (const std::exception& e) {
		HXCOMM_LOG_ERROR(m_logger, "Error during word execution: " << e.what());
		m_sessions_with_failed_reinit_schedule_out.insert(session_id);
		teardown();
	}
}

template <typename Connection>
std::optional<typename QuiggeldyWorker<Connection>::user_session_type>
QuiggeldyWorker<Connection>::verify_user(std::string const& user_data)
{
	boost::uuids::string_generator string_gen;
	boost::uuids::uuid session_uuid;
	std::string uid_like;

	HXCOMM_LOG_TRACE(m_logger, "Verifying " << user_data);
#ifdef USE_MUNGE_AUTH
	if (m_use_munge) {
		munge_err_t err;
		uid_t uid;
		gid_t gid;

		void* buffer;
		int buffer_len;

		// values taken from slurm code
		const auto wait_attempt_delay = std::chrono::milliseconds(200);
		const size_t num_retries = 1000;

		auto munge_ctx = munge_ctx_setup();

		for (size_t tries = 0; tries < num_retries; ++tries) {
			err = munge_decode(user_data.c_str(), munge_ctx, &buffer, &buffer_len, &uid, &gid);
			if (err == EMUNGE_SOCKET) {
				HXCOMM_LOG_TRACE(m_logger, "Munge decode failed, retrying..");
				std::this_thread::sleep_for(wait_attempt_delay);
				continue;
			} else if (err == EMUNGE_CRED_REPLAYED) {
				// TODO: If all calls set user data anew, we could drop this.
				//       lib-rcf supports this now -> TEST! See #3963.
				HXCOMM_LOG_TRACE(
				    m_logger,
				    "Munge decoded replayed credentials, this is fine for reinit submission..");
				break;
			} else {
				// any other error (or success) leads to direct abort
				break;
			}
		}

		// we might not have access to the user name -> simply convert uid to string
		uid_like = std::to_string(uid);

		munge_ctx_destroy(munge_ctx);

		if (err != EMUNGE_SUCCESS) {
			// any other error leads to direct abort
			HXCOMM_LOG_ERROR(
			    m_logger,
			    "User could not be verified. Is munge running? ERROR: " << munge_strerror(err));

			return std::nullopt;
		}
		try {
			session_uuid = string_gen(static_cast<char*>(buffer));
			std::free(buffer);
		} catch (const std::runtime_error&) {
			HXCOMM_LOG_WARN(m_logger, "Invalid session id: " << buffer);
			std::free(buffer);
			return std::nullopt;
		}
		HXCOMM_LOG_DEBUG(m_logger, "Verified via munge: " << uid_like << "@" << session_uuid);
	}
#else
	if (false) {
	}
#endif
	else {
		// If we de not use munge, the client sends user-id and session-name separated by a colon.
		std::string delimiter(":");
		auto session_idx = user_data.find(delimiter);

		if (session_idx == std::string::npos ||
		    session_idx + delimiter.length() >= user_data.length()) {
			HXCOMM_LOG_WARN(m_logger, "Invalid user data: " << user_data);
			return std::nullopt;
		}

		auto user_id = user_data.substr(0, session_idx);
		auto session_id = user_data.substr(
		    session_idx + delimiter.length(),
		    user_data.length() - delimiter.length() - session_idx);

		try {
			session_uuid = string_gen(session_id);
		} catch (const std::runtime_error&) {
			HXCOMM_LOG_WARN(m_logger, "Invalid session id in: " << user_data);
			return std::nullopt;
		}

		if (m_public_key) {
			if (!m_session_tokens.contains(session_id) || !m_session_tokens.at(session_id)) {
				return std::nullopt;
			} else {
				if (!m_token_encryption) {
					throw std::runtime_error(
					    "No encryption method for user token specified, but trying to decode.");
				}
				try {
					verify_jwt(
					    *m_session_tokens.at(session_id), *m_public_key,
					    m_token_expiration_grace_time, std::nullopt, *m_token_encryption);
				} catch (std::logic_error&) {
					return std::nullopt;
				}
			}
		}

		uid_like = user_id;
		HXCOMM_LOG_DEBUG(m_logger, "Verified WITHOUT munge: " << uid_like << "@" << session_uuid);
	}

	return std::make_optional(std::make_pair(uid_like, session_uuid));
}

template <typename Connection>
void QuiggeldyWorker<Connection>::set_enable_mock_mode(bool mode_enable)
{
	m_mock_mode = mode_enable;
}

template <typename Connection>
void QuiggeldyWorker<Connection>::set_enable_allocate_license(bool enable_license_alloc)
{
	if (has_slurm_allocation() && !enable_license_alloc) {
		// release slurm allocation if we go into mock mode with allocated license
		slurm_allocation_release();
	}
	m_allocate_license = enable_license_alloc;
}

template <typename Connection>
void QuiggeldyWorker<Connection>::set_slurm_partition(std::string partition)
{
	m_slurm_partition = partition;
}

template <typename Connection>
void QuiggeldyWorker<Connection>::set_use_munge(bool use_munge)
{
	m_use_munge = use_munge;
#ifndef USE_MUNGE_AUTH
	if (m_use_munge) {
		HXCOMM_LOG_WARN(
		    m_logger, "hxcomm was not compiled with munge support, munge will not be used!");
	}
#endif
}

template <typename Connection>
bool QuiggeldyWorker<Connection>::get_use_munge() const
{
	return m_use_munge;
}

template <typename Connection>
bool QuiggeldyWorker<Connection>::get_use_jwt() const
{
	return static_cast<bool>(m_public_key);
}

template <typename Connection>
std::string QuiggeldyWorker<Connection>::get_unique_identifier(
    std::optional<std::string> hwdb_path) const
{
	// TODO (#3964): Make get_unique_identifier static member function that
	// can be called without active hardware connection maybe requiring changes
	// to Connection/HWDB things.
	if (m_connection) {
		auto unique_identifier = m_connection->get_unique_identifier(hwdb_path);
		HXCOMM_LOG_DEBUG(m_logger, "Requested unique identifier: " << unique_identifier);
		return unique_identifier;
	} else {
		throw std::runtime_error("Requested unique identifier of uninitialized connection.");
	}
}

template <typename Connection>
HwdbEntry QuiggeldyWorker<Connection>::get_hwdb_entry() const
{
	// TODO (#3964): Make get_hwdb_entry static member function that
	// can be called without active hardware connection maybe requiring changes
	// to Connection/HWDB things.
	if (m_connection) {
		return m_connection->get_hwdb_entry();
	} else {
		throw std::runtime_error("Requested hwdb entry of uninitialized connection.");
	}
}

template <typename Connection>
std::string QuiggeldyWorker<Connection>::get_bitfile_info() const
{
	if (m_connection) {
		auto bitfile_info = m_connection->get_bitfile_info();
		HXCOMM_LOG_DEBUG(m_logger, "Requested bitfile info: " << bitfile_info);
		return bitfile_info;
	} else {
		throw std::runtime_error("Requested bitfile info of uninitialized connection.");
	}
}

template <typename Connection>
std::string QuiggeldyWorker<Connection>::get_remote_repo_state() const
{
	auto const repo_state = get_remote_repo_state();
	HXCOMM_LOG_DEBUG(m_logger, "Requested remote repo state: " << repo_state);
	return repo_state;
}

template <typename Connection>
void QuiggeldyWorker<Connection>::set_delay_after_connection_attempt(
    std::chrono::milliseconds delay)
{
	m_delay_after_connection_attempt = delay;
}

template <typename Connection>
std::chrono::milliseconds QuiggeldyWorker<Connection>::get_delay_after_connection_attempt() const
{
	return m_delay_after_connection_attempt;
}

template <typename Connection>
void QuiggeldyWorker<Connection>::set_max_num_connection_attemps(std::size_t num)
{
	m_max_num_connection_attempts = num;
}

template <typename Connection>
std::size_t QuiggeldyWorker<Connection>::get_max_num_connection_attemps() const
{
	return m_max_num_connection_attempts;
}

template <typename Connection>
std::string QuiggeldyWorker<Connection>::get_slurm_jobname() const
{
	return "chip_alloc_" + get_slurm_license();
}

template <typename Connection>
void QuiggeldyWorker<Connection>::set_slurm_license(std::string license)
{
	m_slurm_license = std::move(license);
}

template <typename Connection>
std::string const& QuiggeldyWorker<Connection>::get_slurm_license() const
{
	return m_slurm_license;
}

} // namespace hxcomm
