#pragma once

#ifdef USE_MUNGE_AUTH
#include <munge.h>
#endif

#include "hxcomm/common/quiggeldy_interface_types.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_hash.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <memory>
#include <optional>
#include <string>
#include <type_traits>

namespace log4cxx {

class Logger;

} // namespace log4cxx

namespace hxcomm {
/**
 * Implementation of server-side experiment execution.
 *
 * Set up and used by quiggeldy and not intended to be used in user-code.
 */
template <typename Connection>
class QuiggeldyWorker
{
public:
	using interface_types = quiggeldy_interface_types<typename Connection::message_types>;

	using request_type = typename interface_types::request_type;
	using response_type = typename interface_types::response_type;
	using reinit_type = typename interface_types::reinit_type;

	// pair encoding uid and UUID of the session to differentiate them (one per QuiggeldyConnection)
	using user_session_type = std::pair<std::string, boost::uuids::uuid>;

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
	std::optional<user_session_type> verify_user(std::string const& user_data);

	/**
	 * This function is called by the scheduler whenever there is work to
	 * execute (in our case FPGA words that need to be send and then an answer
	 * to receive) but the chip is currently not used by us.
	 *
	 * The function performs whatever is necessary to run experiments on a
	 * locally attach board. This includes getting a license allocation.
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
	response_type work(request_type const& req);

	/**
	 * This function is called whenever we had to relinquish control of our
	 * hardware resource and the user specified a reinit-program to be loaded
	 * prior to the next work-unit being executed.
	 */
	void perform_reinit(reinit_type const&);

	/**
	 * This function is called whenever we have to relinquish control of our
	 * hardware resource and the user specified a reinit-program to be snapshotted
	 * after the previous work-unit was executed.
	 */
	void perform_reinit_snapshot(reinit_type&);

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
	 * Set or unset if the worker should perform its work with a license
	 * allocation.
	 *
	 * @param enable_license_allocj Whether or not to perform work with a
	 * license allocation.
	 */
	void set_enable_allocate_license(bool enable_license_alloc);

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

	/**
	 * Get whether worker expects to decode user data via munge.
	 *
	 * @return Whether worker expects to decode user data via munge.
	 */
	bool get_use_munge() const;

	/**
	 * Get unique identifier from hwdb.
	 * @param hwdb_path Optional path to hwdb
	 * @return Unique identifier
	 */
	std::string get_unique_identifier(std::optional<std::string> hwdb_path = std::nullopt) const;

	/**
	 * Get bitfile information.
	 * @return Bitfile info
	 */
	std::string get_bitfile_info() const;

	/**
	 * Set the amount of time to wait, if connecting to the "real" backend fails.
	 */
	void set_delay_after_connection_attempt(std::chrono::milliseconds delay);

	/**
	 * Get the amount of time to wait, if connecting to the "real" backend fails.
	 *
	 * @return Number of millisconds between attempts.
	 */
	std::chrono::milliseconds get_delay_after_connection_attempt() const;

	/**
	 * Set how many connection attempts to the "real" backend are made prior to failing hard.
	 *
	 * @param num Number of attempts to set.
	 */
	void set_max_num_connection_attemps(std::size_t num);

	/**
	 * Get how many connection attempts to the "real" backend are made prior to failing hard.
	 *
	 * @return Number of attempts
	 */
	std::size_t get_max_num_connection_attemps() const;

	/**
	 * Set slurm license to allocate.
	 *
	 * @param license Slurm license to acquire in setup().
	 */
	void set_slurm_license(std::string license);

	/**
	 * Get slurm license that is allocated by worker.
	 *
	 * @return Slurm license to acquire in setup().
	 */
	std::string const& get_slurm_license() const;

protected:
	using connection_init_type = typename Connection::init_parameters_type;

	/**
	 * Create a new connection, closing the old one in the process.
	 */
	void setup_connection();

	/**
	 * Check if response contains a timeout response, resetting the held connection if so.
	 *
	 * @param response to check for timeouts.
	 */
	void check_for_timeout(typename response_type::first_type const& response);

	std::string get_slurm_jobname() const;

	/**
	 * Helper function to execute slurm binaries.
	 */
	template <typename... Args>
	void exec_slurm_binary(char const* binary_name, Args&&... args);

	void slurm_allocation_acquire();
	void slurm_allocation_release();

	bool has_slurm_allocation();

	connection_init_type m_connection_init; /// Initial parameters for connection
	std::string m_slurm_partition;          /// Which slurm partition to allocate in.
	std::string m_slurm_license;            /// Explicit slurm license to use for allocation.
	bool m_has_slurm_allocation;
	bool m_allocate_license;
	bool m_mock_mode;
	std::unique_ptr<Connection> m_connection; /// Wrapped connection object.
	log4cxx::Logger* m_logger;
	bool m_use_munge;
	std::size_t m_max_num_connection_attempts;
	std::chrono::milliseconds m_delay_after_connection_attempt;

	static constexpr char default_slurm_partition[]{"cube"};
};

} // namespace hxcomm

#include "hxcomm/common/quiggeldy_worker.tcc"
