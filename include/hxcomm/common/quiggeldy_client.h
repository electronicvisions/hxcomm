#pragma once

#include "hxcomm/common/connection.h"
#include "hxcomm/common/execute_messages.h"
#include "hxcomm/common/stream.h"
#include "hxcomm/common/target_restriction.h"

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

#ifdef __clang__

/**
 * Because clang is unable to treat templates with default parameters as
 * templates with fewer arguments (i.e. std::vector only needs one template
 * argument the rest are default values), we need a helper struct that maps
 * vector to a template with exactly one argument.
 */
namespace detail {

template <typename T>
struct workaround_clang_vector : std::vector<T>
{};

} // namespace detail

#endif

/**
 * HostARQ connection class.
 * Establish and hold HostARQ connection to FPGA.
 * Provide convenience functions for sending and receiving UT messages.
 * @tparam ConnectionParameter UT message parameter for connection
 */
template <
    typename ConnectionParameter,
    typename RcfClient,
#ifdef __clang__
    template <typename> class Sequence = detail::workaround_clang_vector
#else
    template <typename> class Sequence = std::vector
#endif
    >
class QuiggeldyClient
{
public:
	HXCOMM_EXPOSE_MESSAGE_TYPES(ConnectionParameter)

	using quiggeldy_request_type = Sequence<send_message_type>;
	using quiggeldy_response_type = Sequence<receive_message_type>;

	using connect_parameters_type = std::tuple<std::string, uint16_t>;

	/**
	 * Connects to server while getting IP and port from environment.
	 */
	QuiggeldyClient();

	/**
	 * Connects to server with explicit ip/port.
	 * @param ip Target ip address.
	 * @param port Target port.
	 */
	QuiggeldyClient(std::string ip, uint16_t port);

	/**
	 * Connect via tuple of ip/port.
	 */
	QuiggeldyClient(connect_parameters_type const& params);

	/**
	 * Default move constructor.
	 */
	QuiggeldyClient(QuiggeldyClient&&) noexcept;

	/**
	 * Assignment operator.
	 */
	QuiggeldyClient& operator=(QuiggeldyClient&&) noexcept;

	/**
	 * Destruct connection to FPGA joining all receive threads.
	 */
	~QuiggeldyClient();

	/**
	 * Get whether connection supports given target restriction.
	 * @param restriction Restriction to check support for
	 * @return Boolean support value
	 */
	bool supports(TargetRestriction restriction) const;

	/**
	 * Set maximum number of connection attempts.
	 * @param num Number of connection attempts.
	 */
	void set_connection_attempts_max(size_t num);

	/**
	 * Get maximum number of connection attempts.
	 * @return Number of connection attempts.
	 */
	size_t get_connection_attempts_max();

	/**
	 * Set wait duration after every connection attempt number of connection attempts.
	 * @param ms Wait duration after every connection attempt.
	 */
	void set_connection_attempt_wait_after(std::chrono::milliseconds ms);

	/**
	 * Get wait duration after every connection attempt number of connection attempts.
	 * @return Wait duration after every connection attempt.
	 */
	std::chrono::milliseconds get_connection_attempt_wait_after();

	/**
	 * Set whether or not to encode user information using munge.
	 *
	 * This is needed if the remote side is also using munge (i.e., regular
	 * operations), but should be disabled in testing environments running
	 * without munge.
	 *
	 * This option has no effect if fisch was compiled with `--without-munge`.
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
	 * @return If munge should be used (true) or not (false).
	 */
	bool get_use_munge();

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

protected:
	friend Stream<QuiggeldyClient>;

	using rcf_client_type = RcfClient;

	/**
	 * Set up the RCF-client, i.e., establish a connection to the server.
	 *
	 * @param ip IP of RCF server.
	 * @param port Port of RCF server.
	 */
	void setup_client(std::string const& ip, uint16_t port);

	/**
	 * Send the given request to the server and block until response is ready.
	 * @param req Request which is sent to the server.
	 * @return Response containing FPGA words from the connection.
	 */
	quiggeldy_response_type submit_blocking(quiggeldy_request_type const& req);

	/**
	 * Prepare to send request. Needs to be called before sending the actual
	 * request via client.
	 *
	 * Sets up potential munge authentication.
	 */
	void prepare_request();

	connect_parameters_type get_connect_params_from_env();

	size_t m_connection_attempt_num_max;
	std::chrono::milliseconds m_connection_attempt_wait_after;
	std::unique_ptr<rcf_client_type> m_client;
	log4cxx::Logger* m_logger;
	bool m_use_munge;
#ifdef USE_MUNGE_AUTH
	munge_ctx_t m_munge_ctx;
#endif
};

namespace detail {

template <typename... ConnectionParameters, template <typename> class Sequence>
struct ExecutorMessages<QuiggeldyClient<ConnectionParameters...>, Sequence>;

} // namespace detail

template <typename ConnectionParameter, typename RcfClient, template <typename> class Sequence>
class Stream<QuiggeldyClient<ConnectionParameter, RcfClient, Sequence> >;

} // namespace hxcomm

#ifndef __GENPYBIND__
#include "hxcomm/common/quiggeldy_client.tcc"
#endif
