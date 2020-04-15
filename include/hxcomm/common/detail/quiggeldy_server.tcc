#include "hxcomm/common/detail/quiggeldy_server.h"
#include "hxcomm/common/execute_messages.h"
#include "hxcomm/common/logger.h"
#include "hxcomm/common/quiggeldy_common.h"

#include "hate/memory.h"

#include "slurm/vision_defines.h"

#include "logger.h"
#include "logging_ctrl.h"

#include <chrono>
#include <cstdlib>
#include <functional>
#include <sstream>
#include <thread>
#include <unistd.h>
#include <unordered_map> // needed for std::hash<std::string>
#include <sys/wait.h>

namespace hxcomm::detail {

using namespace std::literals::chrono_literals;

template <typename Connection>
template <typename... Args>
QuiggeldyWorker<Connection>::QuiggeldyWorker(Args&&... args) :
    m_connection_init(std::forward<Args>(args)...),
    m_has_slurm_allocation(false),
    m_mock_mode(false),
    m_logger(log4cxx::Logger::getLogger("QuiggeldyWorker")),
    m_use_munge(true)
{
	if (m_logger->isEnabledFor(log4cxx::Level::getTrace())) {
		std::stringstream ss;
		ss << "Passing " << (sizeof...(Args)) << " arguments to connection:";
		((ss << " " << args), ...);
		HXCOMM_LOG_TRACE(m_logger, ss.str())
	}
	char const* env_partition = std::getenv(vision_quiggeldy_partition);
	if (env_partition == nullptr) {
		m_slurm_partition = default_slurm_partition;
	} else {
		m_slurm_partition = env_partition;
	}
}

template <typename Connection>
QuiggeldyWorker<Connection>::~QuiggeldyWorker() = default;

template <typename Connection>
void QuiggeldyWorker<Connection>::slurm_allocation_acquire()
{
	// prevent error if we already have slurm allocation
	if (has_slurm_allocation()) {
		return;
	}
	HXCOMM_LOG_DEBUG(
	    m_logger, "Getting slurm allocation for " << get_slurm_jobname().c_str() << ".");
	HXCOMM_LOG_TRACE(
	    m_logger, "Running: /usr/local/bin/salloc salloc -p "
	                  << m_slurm_partition.c_str() << " --no-shell --mem 0 --gres "
	                  << get_slurm_gres().c_str() << " -J " << get_slurm_jobname().c_str());
	int status;

	int pid = fork();
	if (pid) {
		waitpid(pid, &status, 0); // wait for the child to exit
	} else {
		// TODO: Link to actual libslurm instead of forking
		execlp(
		    "salloc", "salloc", "-p", m_slurm_partition.c_str(), "--no-shell", "--gres",
		    get_slurm_gres().c_str(), "--mem", "0M", "-J", get_slurm_jobname().c_str(), NULL);
		exit(0);
	}

	if (status) {
		throw std::logic_error("slurm allocation failed");
	}
	m_has_slurm_allocation = true;
	HXCOMM_LOG_DEBUG(
	    m_logger,
	    "Slurm allocation for " << get_slurm_jobname().c_str() << " successfully acquired.");
}

template <typename Connection>
void QuiggeldyWorker<Connection>::setup()
{
	if (!m_mock_mode) {
		if (m_allocate_gres) {
			slurm_allocation_acquire();
		}
		HXCOMM_LOG_TRACE(m_logger, "Setting up local connection.");
		// TODO have the experiment control timeout (e.g. when the board is unresponsive)
		auto new_connection = hate::memory::make_unique_from_tuple<Connection>(m_connection_init);
		m_connection.swap(new_connection);
	} else {
		HXCOMM_LOG_DEBUG(m_logger, "Operating in mock-mode - no connection allocated.");
	}
	HXCOMM_LOG_TRACE(m_logger, "setup() completed!");
}

template <typename Connection>
void QuiggeldyWorker<Connection>::teardown()
{
	if (!m_mock_mode) {
		m_connection.reset();
		if (m_allocate_gres) {
			slurm_allocation_release();
		}
	}
	HXCOMM_LOG_DEBUG(m_logger, "teardown() completed!");
}

template <typename...>
struct all_char_const_ptr;

template <>
struct all_char_const_ptr<> : std::true_type
{};

template <typename Head, typename... Tail>
struct all_char_const_ptr<Head, Tail...>
    : std::integral_constant<
          bool,
          std::is_convertible<Head, char const*>::value && all_char_const_ptr<Tail...>::value>
{};

template <typename Connection>
template <typename... Args>
void QuiggeldyWorker<Connection>::exec_slurm_binary(char const* binary_name, Args&&... args)
{
	static_assert(
	    all_char_const_ptr<Args...>::value,
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
typename QuiggeldyWorker<Connection>::quiggeldy_response_type QuiggeldyWorker<Connection>::work(
    typename QuiggeldyWorker<Connection>::quiggeldy_request_type const& req)
{
	if (m_mock_mode) {
		HXCOMM_LOG_DEBUG(m_logger, "Running mock-experiment!");
		return quiggeldy_response_type();
	}

	HXCOMM_LOG_TRACE(m_logger, "Executing program!");
	try {
		return execute_messages(*m_connection, req);
	} catch (const std::exception& e) {
		// TODO: Implement proper exception handling
		teardown();
		HXCOMM_LOG_ERROR(m_logger, "Error during word execution: " << e.what());
		throw;
	}
}

template <typename Connection>
std::optional<size_t> QuiggeldyWorker<Connection>::verify_user(std::string const& user_data)
{
	HXCOMM_LOG_TRACE(m_logger, "Verifying " << user_data);
#ifdef USE_MUNGE_AUTH
	if (m_use_munge) {
		munge_err_t err;
		uid_t uid;
		gid_t gid;

		// values taken from slurm code
		const auto wait_attempt_delay = std::chrono::milliseconds(200);
		const size_t num_retries = 1000;

		auto munge_ctx = munge_ctx_setup();

		for (size_t tries = 0; tries < num_retries; ++tries) {
			err = munge_decode(user_data.c_str(), munge_ctx, NULL, NULL, &uid, &gid);
			if (err == EMUNGE_SOCKET) {
				HXCOMM_LOG_TRACE(m_logger, "Munge decode failed, retrying..");
				std::this_thread::sleep_for(wait_attempt_delay);
				continue;
			} else {
				// any other error (or success) leads to direct abort
				break;
			}
		}

		munge_ctx_destroy(munge_ctx);

		if (err != EMUNGE_SUCCESS) {
			// any other error leads to direct abort
			HXCOMM_LOG_ERROR(
			    m_logger,
			    "User could not be verified. Is munge running? ERROR: " << munge_strerror(err));

			return std::nullopt;
		} else {
			return std::make_optional(uid);
		}
	}
#else
	if (false) {
	}
#endif
	else {
		// We hash in order to convert from string -> size_t in a reliable
		// manner (same user data -> same hash).
		// This way, even without munge, if every user faithfully sends his
		// encoded user name we can still identify users and use that
		// information in scheduling.
		// No attempt to verify user data is made though, so one user could
		// pretend to be several and execute more experiments comparatively.
		return std::make_optional(std::hash<std::string>{}(user_data));
	}
}

template <typename Connection>
std::string QuiggeldyWorker<Connection>::get_slurm_gres()
{
	std::stringstream ss;

	ss << "quiggeldy";

	auto paste = [&ss](auto&& arg) { ss << "_" << arg; };

	std::apply([&ss, &paste](auto&&... arg) { (..., paste(arg)); }, m_connection_init);

	return ss.str();
}

template <typename Connection>
void QuiggeldyWorker<Connection>::set_enable_mock_mode(bool mode_enable)
{
	m_mock_mode = mode_enable;
}

template <typename Connection>
void QuiggeldyWorker<Connection>::set_enable_allocate_gres(bool enable_gres_alloc)
{
	if (has_slurm_allocation() && !enable_gres_alloc) {
		// release slurm allocation if we go into mock mode with allocated gres
		slurm_allocation_release();
	}
	m_allocate_gres = enable_gres_alloc;
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
		    m_logger, "fisch was not compiled with munge support, munge will not be used!");
	}
#endif
}

template <typename Connection>
std::string QuiggeldyWorker<Connection>::get_slurm_jobname()
{
	return "chip_alloc_" + get_slurm_gres();
}

} // namespace hxcomm::detail
