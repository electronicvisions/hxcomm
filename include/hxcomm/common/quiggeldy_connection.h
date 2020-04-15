#pragma once

#include "hxcomm/common/connection.h"
#include "hxcomm/common/connection_time_info.h"
#include "hxcomm/common/execute_messages.h"
#include "hxcomm/common/quiggeldy_future.h"
#include "hxcomm/common/quiggeldy_interface_types.h"
#include "hxcomm/common/stream.h"
#include "hxcomm/common/stream_rc.h"
#include "hxcomm/common/target.h"

#include "rcf-extensions/sequence-number.h"

#include "RCF/RCF.hpp"

#include <chrono>
#include <string>
#include <vector>

#ifdef USE_MUNGE_AUTH
#include <munge.h>
#endif

namespace log4cxx {
class Logger;

} // namespace log4cxx

namespace hxcomm {

/**
 * QuiggeldyConnection class.
 * Establish and hold connection to a remote quiggeldy instance that itself
 * proxies another connection.
 * @tparam ConnectionParameter UT message parameter for connection
 * @tparam RcfClient RCF-based client that is needed to connect to quiggeldy instance.
 */
template <typename ConnectionParameter, typename RcfClient>
class QuiggeldyConnection
{
public:
	HXCOMM_EXPOSE_MESSAGE_TYPES(ConnectionParameter)

	static constexpr char name[]{"QuiggeldyConnection"};

	/**
	 * Connects to server while getting IP and port from environment.
	 */
	QuiggeldyConnection();

	/**
	 * Connects to server with explicit ip/port.
	 * @param ip Target ip address.
	 * @param port Target port.
	 */
	QuiggeldyConnection(std::string ip, uint16_t port);

	using connect_parameters_type = std::tuple<std::string, uint16_t>;
	/**
	 * Connect via tuple of ip/port.
	 */
	QuiggeldyConnection(connect_parameters_type const& params);

	/**
	 * Move constructor.
	 */
	QuiggeldyConnection(QuiggeldyConnection&&);

	/**
	 * No copies allowed.
	 */
	QuiggeldyConnection(QuiggeldyConnection const&) = delete;

	/**
	 * No copies allowed.
	 */
	QuiggeldyConnection& operator=(QuiggeldyConnection const&) = delete;

	/**
	 * Destruct connection to FPGA joining all receive threads.
	 */
	~QuiggeldyConnection() = default;

	constexpr static auto supported_targets = {Target::hardware};

	using interface_types = quiggeldy_interface_types<ConnectionParameter>;

	using rcf_client_type = RcfClient;

	using future_type = QuiggeldyFuture<typename interface_types::response_type, rcf_client_type>;

	/**
	 * Set maximum number of connection attempts.
	 * @param num Number of connection attempts.
	 */
	void set_connection_attempts_max(size_t num);

	/**
	 * Get maximum number of connection attempts.
	 * @return Number of connection attempts.
	 */
	size_t get_connection_attempts_max() const;

	/**
	 * Set wait duration after every connection attempt number of connection attempts.
	 * @param ms Wait duration after every connection attempt.
	 */
	void set_connection_attempt_wait_after(std::chrono::milliseconds ms);

	/**
	 * Get wait duration after every connection attempt number of connection attempts.
	 * @return Wait duration after every connection attempt.
	 */
	std::chrono::milliseconds get_connection_attempt_wait_after() const;

	/**
	 * Set whether or not to encode user information using munge.
	 *
	 * This is needed if the remote side is also using munge (i.e., regular
	 * operations), but should be disabled in testing environments running
	 * without munge.
	 *
	 * This option has no effect if compiling with `--without-munge`.
	 *
	 * @param value If munge should be used (true) or not (false).
	 */
	void set_use_munge(bool value);

	/**
	 * Get whether or not to encode user information using munge.
	 *
	 * This is needed if the remote side is also using munge (i.e., regular
	 * operations), but should be disabled in testing environments running
	 * without munge.
	 *
	 * This option has no effect if fisch was compiled with `--without-munge`.
	 *
	 * @return Whether munge should be used (true) or not (false).
	 */
	bool get_use_munge() const;

	/**
	 * When running experiments over quiggeldy - if running blocking - RCF
	 * needs to keep the connection open for as long as an experiment could
	 * theoretically be running.
	 *
	 * For now just set it to one month.
	 * If needed, this should be set to a more reasonable constant in the future.
	 */
	static constexpr std::chrono::hours program_runtime_max =
	    std::chrono::hours(24 * 30); // one month

	/**
	 * Return whether this instance submits packages out-of-order, i.e., if
	 * submitted asynchronous requests could be executed out of order.
	 *
	 * Note: In case of out-of-order execution, single submitted work packages
	 * should correspond to a full program.
	 */
	bool is_out_of_order() const;

	/**
	 * Set the QuiggeldyConnection to set out-of-order work units. This can speed
	 * up execution times if each playback-program does not depend on other
	 * playback programs. Plus, it might make learning more robust.
	 */
	void set_out_of_order();

	/**
	 * Get time information.
	 * @return Time information
	 */
	ConnectionTimeInfo get_time_info() const;

	/**
	 * Get unique identifier from hwdb.
	 * @param hwdb_path Optional path to hwdb
	 * @return Unique identifier
	 */
	std::string get_unique_identifier(std::optional<std::string> hwdb_path = std::nullopt) const;

protected:
	// needs to be first so it is initialized first and RCF-functionality is set up
	RCF::RcfInit m_rcf_init_deinit;

	friend StreamRC<QuiggeldyConnection>;

	/**
	 * Set up the RCF-client, i.e., establish a connection to the server.
	 *
	 * Takes its parameters from `m_connect_parameters`.
	 */
	std::unique_ptr<rcf_client_type> setup_client() const;

	/**
	 * Send the given request to the server and block until response is ready.
	 *
	 * This function is reentrant.
	 *
	 * @param req Request which is sent to the server.
	 * @return Response containing FPGA words from the connection.
	 */
	typename interface_types::response_type submit_blocking(
	    typename interface_types::request_type const& req);

	/**
	 * Send the given request to the server and block until response is ready.
	 * @param req Request which is sent to the server.
	 * @return Response containing FPGA words from the connection.
	 */
	future_type submit_async(typename interface_types::request_type const& req);

	/**
	 * Prepare user data in connection to authenticate request on the server
	 * side. Called by setup_client().
	 *
	 * Sets up potential munge authentication.
	 */
	void set_user_data(std::unique_ptr<rcf_client_type>&) const;

	/**
	 * Get next sequence number for a new request.
	 */
	rcf_extensions::SequenceNumber next_sequence_number();

	connect_parameters_type get_connect_params_from_env();

	/**
	 * Lock guard for time info.
	 */
	std::lock_guard<std::mutex> lock_time_info() const;

	void accumulate_time_info(ConnectionTimeInfo const& delta);

	template <typename Submitter>
	auto submit(Submitter const&);

	connect_parameters_type m_connect_parameters;
	size_t m_connection_attempt_num_max;
	std::chrono::milliseconds m_connection_attempt_wait_after;
	log4cxx::Logger* m_logger;
	bool m_use_munge;
	std::mutex m_mutex_sequence_num;
	rcf_extensions::SequenceNumber m_sequence_num;

	mutable std::mutex m_mutex_time_info;
	ConnectionTimeInfo m_time_info;
};

namespace detail {

template <typename ConnectionParameters, typename RcfClient>
struct ExecutorMessages<QuiggeldyConnection<ConnectionParameters, RcfClient>>;

// Indiate that QuiggeldyConnection is not expected to support full stream interface.
template <typename ConnectionParameter, typename RcfClient>
struct supports_full_stream_interface<QuiggeldyConnection<ConnectionParameter, RcfClient>>
    : std::false_type
{};

} // namespace detail

} // namespace hxcomm

//#ifndef __GENPYBIND__
#include "hxcomm/common/quiggeldy_connection.tcc"
//#endif
