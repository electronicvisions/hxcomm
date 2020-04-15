#pragma once

#include "rcf-extensions/round-robin.h"

#ifdef USE_MUNGE_AUTH
#include <munge.h>
#endif

#include <memory>
#include <optional>
#include <string>
#include <type_traits>

namespace log4cxx {

class Logger;

} // namespace log4cxx

namespace hxcomm::detail {

/**
 * Implementation of server-side experiment execution.
 *
 * Set up and used by quiggeldy and not intended to be used in user-code.
 */
template <typename Connection>
class QuiggeldyWorker
{
public:
	using quiggeldy_request_type = std::vector<typename Connection::send_message_type>;
	using quiggeldy_response_type = std::vector<typename Connection::receive_message_type>;

	/**
	 * Set up the Quick Queue Server.
	 */
	template <typename... Args>
	QuiggeldyWorker(Args&&...);

	QuiggeldyWorker(QuiggeldyWorker&& other) = default;
	QuiggeldyWorker& operator=(QuiggeldyWorker&& other) = default;

	QuiggeldyWorker(QuiggeldyWorker const& other) = delete;
	QuiggeldyWorker& operator=(QuiggeldyWorker const& other) = delete;

	~QuiggeldyWorker();

	/**
	 * Verify the authenticity of the given user.
	 * @param user_data User data supplied via RCF. If munge support is enabled
	 * (via USE_MUNGE_AUTH), it should be encoded using munge.
	 * @return If user is authenticated, return identifying hash with which
	 * fair round-robin scheduling is ensured.
	 */
	std::optional<size_t> verify_user(std::string const& user_data);

	/**
	 * This function is called by the scheduler whenever there is work to
	 * execute (in our case FPGA words that need to be send and then an answer
	 * to receive) but the chip is currently not used by us.
	 *
	 * The function performs whatever is necessary to run experiments on a
	 * locally attach board. This includes getting a gres allocation.
	 *
	 * NOTE: If the GRES is already allocated in SLURM, this function will
	 * block until it becomes available!
	 */
	void setup();

	/**
	 * This function is called by the scheduler to actually evaluate the FPGA words
	 * (send data to chip, wait, retrieve response from chip).
	 *
	 * @param req FPGA words to be sent to the chip for this job.
	 */
	quiggeldy_response_type work(quiggeldy_request_type const& req);

	/**
	 * This function is run whenever the server releases control (and any
	 * associated slurm resources).
	 */
	void teardown();

	/**
	 * Set or unset the worker into mock-mode.
	 * If in mock-mode, no communication with any hardware is attempted and empty results are
	 * returned.

	 * @param mode_enable Whether or not to enable mock mode.
	 */
	void set_enable_mock_mode(bool mode_enable);

	/**
	 * Set or unset if the worker should perform its work with a gres allocation.
	 *
	 * @param enable_gres_alloc Whether or not to perform work with a gres allocation.
	 */
	void set_enable_allocate_gres(bool enable_gres_alloc);

	/**
	 * Set slurm partition. (If unset defaults to "cube".)
	 * @param partition Which slurm partition to use in allocations.
	 */
	void set_slurm_partition(std::string partition);

	/**
	 * Set whether or not to verify user information using munge.
	 *
	 * This option has no effect if fisch was compiled with `--without-munge`.
	 *
	 * @param use_munge If munge should be used (true) or not (false).
	 */
	void set_use_munge(bool use_munge);

protected:
	using connection_init_type = typename Connection::init_parameters_type;

	// methods
	std::string get_slurm_jobname();
	std::string get_slurm_gres();

	/**
	 * Helper function to execute slurm binaries.
	 */
	template <typename... Args>
	void exec_slurm_binary(char const* binary_name, Args&&... args);

	void slurm_allocation_acquire();
	void slurm_allocation_release();

	bool has_slurm_allocation();

	// members
	connection_init_type m_connection_init; /// Initial parameters for connection
	std::string m_slurm_partition;          /// Which slurm partition to allocate in.
	bool m_has_slurm_allocation;
	bool m_allocate_gres;
	bool m_mock_mode;
	std::unique_ptr<Connection> m_connection; /// Wrapped connection object.
	log4cxx::Logger* m_logger;
	bool m_use_munge;
#ifdef USE_MUNGE_AUTH
	munge_ctx_t m_munge_ctx;
#endif

	// defaults
	static const std::string default_slurm_partition;
}; // QuiggeldyWorker

template <typename Connection>
const std::string QuiggeldyWorker<Connection>::default_slurm_partition = "cube";

template <typename Connection, typename RcfInterface>
using QuiggeldyServer =
    rcf_extensions::RoundRobinScheduler<QuiggeldyWorker<Connection>, RcfInterface>;

} // namespace hxcomm::detail

#include "hxcomm/common/detail/quiggeldy_server.tcc"
