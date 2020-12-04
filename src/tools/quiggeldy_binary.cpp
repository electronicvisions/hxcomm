#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <optional>
#include <utility>
#include <variant>
#include <boost/program_options.hpp>

#include "pthread.h"
#include "signal.h"
#include "sys/prctl.h"

#include "logger.h"
#include "logging_ctrl.h"

#include "hxcomm/common/cerealization_utmessage.h"
#include "hxcomm/common/quiggeldy_utility.h"
#include "hxcomm/vx/arqconnection.h"
#include "hxcomm/vx/axiconnection.h"
#include "hxcomm/vx/simconnection.h"

#include "hxcomm/common/logger.h"

// include for each target
#include "hxcomm/vx/quiggeldy_server.h"
#include "hxcomm/vx/quiggeldy_worker.h"

// needed to have lambdas as signal handlers
namespace quiggeldy {

#ifdef QUIGGELDY_VERSION_STRING
#define QUIGGELDY_STRINGIFY2(S) #S
#define QUIGGELDY_STRINGIFY(S) QUIGGELDY_STRINGIFY2(S)
static std::string const quiggeldy_version_string = QUIGGELDY_STRINGIFY(QUIGGELDY_VERSION_STRING);
#undef QUIGGELDY_STRINGIFY
#undef QUIGGELDY_STRINGIFY2
#else
static std::string const quiggeldy_version_string = "<undefined>";
#endif

std::function<int(int)> signal_handler;

void* handle_signals_thread(void*);

pthread_t setup_signal_handler_thread()
{
	auto log = log4cxx::Logger::getLogger(std::string("hxcomm.quiggeldy.") + __func__);
	HXCOMM_LOG_TRACE(log, "Setting up signal handler..");

	// set dummy function that ignores all signals to avoid crashing if a
	// signal is received in the short amount of time until the
	// QuiggeldyServer is running
	quiggeldy::signal_handler = []([[maybe_unused]] int sig) {
		auto log = log4cxx::Logger::getLogger(std::string("hxcomm.quiggeldy.") + __func__);
		HXCOMM_LOG_DEBUG(log, "Received signal " << sig << " in dummy signal handler, ignoring..");
		return 0;
	};
	// have all other threads not listen to signals
	sigset_t sigset;
	if (sigfillset(&sigset) != 0) {
		HXCOMM_LOG_FATAL(log, "Failed to fill signal mask: " << strerror(errno));
	}
	if (pthread_sigmask(SIG_BLOCK, &sigset, NULL) != 0) {
		HXCOMM_LOG_FATAL(log, "Could not set signal mask.");
	}

	pthread_t sigthread;
	if (pthread_create(&sigthread, NULL, handle_signals_thread, NULL) != 0) {
		HXCOMM_LOG_FATAL(log, "Could not create signal handling thread.");
	}

	HXCOMM_LOG_TRACE(log, "Signal handling set up.");

	return sigthread;
}

void* handle_signals_thread(void*)
{
	auto log = log4cxx::Logger::getLogger(std::string("hxcomm.quiggeldy.") + __func__);

	HXCOMM_LOG_TRACE(log, "Setting up signal handler thread");

	int sig;
	sigset_t sigset;

	sigfillset(&sigset);
	if (pthread_sigmask(SIG_BLOCK, &sigset, NULL) != 0) {
		HXCOMM_LOG_FATAL(log, "Could not block specific signals in signal handler.");
	}

	while (true) {
		HXCOMM_LOG_TRACE(log, "Waiting for signal..");
		sigwait(&sigset, &sig);
		if (signal_handler(sig) != 0) {
			break;
		}
	}
	return (void*) NULL;
}

struct Config
{
	std::string listen_ip, connect_ip;
	uint16_t listen_port, connect_port;

	std::size_t release_seconds;
	std::size_t timeout_seconds;

	std::size_t log_level;
	std::size_t num_threads_input;
	std::size_t num_threads_output;
	std::size_t num_max_connections;

	bool ignore_parent_death;

	bool mock_mode;
	bool no_allocate_license;
	bool no_munge;

	std::string architecture;
	std::string slurm_partition;
	std::string slurm_license;

	bool backend_arq;
	bool backend_axi;
	bool backend_sim;

	std::size_t delay_after_connect_ms;
	std::size_t max_num_connection_attempts;

	std::size_t period_per_user_seconds;

	static std::size_t hxcomm_log_threshold_at_compile;
};

#ifdef HXCOMM_LOG_THRESHOLD
std::size_t Config::hxcomm_log_threshold_at_compile = HXCOMM_LOG_THRESHOLD;
#else
std::size_t Config::hxcomm_log_threshold_at_compile = 0;
#endif

template <class WorkerT>
void configure(WorkerT& worker, Config& cfg)
{
	auto log = log4cxx::Logger::getLogger("hxcomm.quiggeldy.configure");

	if (cfg.mock_mode) {
		HXCOMM_LOG_INFO(log, "Setting mock-mode.");
	}
	worker.set_enable_mock_mode(cfg.mock_mode);

	if (cfg.no_allocate_license) {
		HXCOMM_LOG_DEBUG(
		    log, "NOT allocating license prior to executing work! This should only be enabled for "
		         "local use!");
	}
	worker.set_enable_allocate_license(!cfg.no_allocate_license);
	HXCOMM_LOG_DEBUG(log, "Setting slurm partition: " << cfg.slurm_partition);
	worker.set_slurm_partition(cfg.slurm_partition);

	if (cfg.delay_after_connect_ms) {
		worker.set_delay_after_connection_attempt(
		    std::chrono::milliseconds{cfg.delay_after_connect_ms});
	}
	if (cfg.max_num_connection_attempts > 0) {
		worker.set_max_num_connection_attemps(cfg.max_num_connection_attempts);
	}

	if (cfg.no_munge) {
		HXCOMM_LOG_DEBUG(log, "Excplicitly disabling munge authentication.");
		worker.set_use_munge(false);
	}
}

template <class ServerT, class WorkerT, class VariantT>
void allocate(std::unique_ptr<VariantT>& server, WorkerT&& worker, Config& cfg)
{
	server.reset(new VariantT{
	    std::in_place_type<ServerT>, RCF::TcpEndpoint(cfg.listen_ip, cfg.listen_port),
	    std::move(worker), cfg.num_threads_input, cfg.num_threads_output, cfg.num_max_connections});
	std::visit(
	    [&cfg](auto& server) {
		    server.set_version(quiggeldy_version_string);
		    server.template bind_to_interface<hxcomm::vx::I_HXCommQuiggeldyVX>();
		    server.set_period_per_user(std::chrono::seconds(cfg.period_per_user_seconds));
	    },
	    *server);
}

} // namespace quiggeldy

namespace po = boost::program_options;

using QuiggeldyWorkerARQ = hxcomm::vx::QuiggeldyWorker<hxcomm::vx::ARQConnection>;
using QuiggeldyWorkerAXI = hxcomm::vx::QuiggeldyWorker<hxcomm::vx::AXIConnection>;
using QuiggeldyWorkerSim = hxcomm::vx::QuiggeldyWorker<hxcomm::vx::SimConnection>;

using QuiggeldyServerARQ = hxcomm::vx::QuiggeldyServer<hxcomm::vx::ARQConnection>;
using QuiggeldyServerAXI = hxcomm::vx::QuiggeldyServer<hxcomm::vx::AXIConnection>;
using QuiggeldyServerSim = hxcomm::vx::QuiggeldyServer<hxcomm::vx::SimConnection>;

using quiggeldy_server_t = std::variant<QuiggeldyServerARQ, QuiggeldyServerAXI, QuiggeldyServerSim>;

int main(int argc, const char* argv[])
{
	quiggeldy::Config cfg{};

	po::options_description desc{
	    "Quiggeldy daemon that handles access to a given hardware resource in a round robin "
	    "fashion, as described in 'Breitwieser 2021'.\n\n"
	    "Allowed options"};
	// clang-format off
	desc.add_options()("help,h", "produce help message")
	("architecture,a", po::value<std::string>()->default_value("vx"),
	 "Which architecture to target? Currently there is only vx supported.")

	("connect-ip,i", po::value<std::string>()->default_value(""), "IP to connect to")
	("connect-port", po::value<uint16_t>(&(cfg.connect_port)),
	 "Port to connect to (if --connection-sim specified)")

	("release,r", po::value<std::size_t>(&(cfg.release_seconds)),
	 "Number of seconds between releases of slurm allocation. A value of zero "
	 "causes quiggeldy to immediately release the slurm allocation as soon as no "
	 "work is present.")

	("connection-arq", po::bool_switch(&(cfg.backend_arq))->default_value(false),
	 "Proxy an ARQ-connection (default mode of operation).")
    ("connection-axi", po::bool_switch(&(cfg.backend_axi))->default_value(false),
     "Proxy an AXI-connection.")
	("connection-sim", po::bool_switch(&(cfg.backend_sim))->default_value(false),
	 "Proxy connection to simulation backend.")

	("delay-after-connect-ms", po::value<std::size_t>(&(cfg.delay_after_connect_ms)),
	 "Number of milliseconds to wait after connection to \"real\" backend failed.")

	("listen-ip", po::value<std::string>()->default_value("0.0.0.0"), "specify listening IP")
	("listen-port,p", po::value<uint16_t>(&(cfg.listen_port))->required(),
	 "specify listening port")

	("loglevel,l",
	 po::value<std::size_t>(&(cfg.log_level))->default_value(2),
	 "specify loglevel [0-TRACE,1-DEBUG,2-INFO,3-WARNING,4-ERROR]")

	("ignore-parent-death",
	 po::bool_switch(&(cfg.ignore_parent_death))->default_value(false),
	 "By default, quiggeldy will terminate itself if its parent process dies. "
	 " Setting --ignore-parent-death disables this behaviour.")

	("max-num-connection-attempts",
	 po::value<std::size_t>(&(cfg.max_num_connection_attempts))->default_value(10),
	 "Maximum number of connection attemps to perform to \"real\" backend prior to exiting.")
	("max-connections,c",
	 po::value<std::size_t>(&(cfg.num_max_connections))->default_value(1 << 15),
	 "Maximum number of allowed connections.")

	("mock-mode", po::bool_switch(&(cfg.mock_mode))->default_value(false),
	 "Operate in mock-mode, i.e., accept connections but return empty results.")
	("no-allocate-license", po::bool_switch(&(cfg.no_allocate_license))->default_value(false),
	 "Do not allocate license prior to running jobs.")
	("no-munge", po::bool_switch(&(cfg.no_munge))->default_value(false),
	 "Do not verify clients using munge.")

	("num-threads-input,n", po::value<std::size_t>(&(cfg.num_threads_input))->default_value(8),
	 "Number of threads handling incoming connections.")
	("num-threads-outputs,m",
	 po::value<std::size_t>(&(cfg.num_threads_output))->default_value(2),
	 "Number of threads handling distribution of results.")

    ("slurm-license", po::value<std::string>()->default_value(""),
     "Slurm license which to allocate (if not disabled via --no-allocate-license).")

	("slurm-partition", po::value<std::string>()->default_value("cube"),
	 "Slurm partition in which to allocate license (if not disabled via --no-allocate-license).")

	("timeout,t", po::value<std::size_t>(&(cfg.timeout_seconds)),
	 "Number of seconds after which quiggeldy shuts down when idling (0=disable).")

	("user-period,u", po::value<std::size_t>(&(cfg.period_per_user_seconds)),
	 "Number of seconds after which we switch from one user to another in case "
	 "there are people waiting.")

    ("version,v", "print version and exit");
	// clang-format on

	// populate vm variable
	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);

	if (vm.count("help")) {
		std::cerr << desc << std::endl;
		return EXIT_SUCCESS;
	}
	if (vm.count("version")) {
		std::cout << "quiggeldy " << quiggeldy::quiggeldy_version_string << std::endl;
		return EXIT_SUCCESS;
	}
	po::notify(vm);

	// extract all string variables from vm because supplying references to
	// them when creating the argument leads to data corruption
	cfg.listen_ip = vm["listen-ip"].as<std::string>();
	cfg.connect_ip = vm["connect-ip"].as<std::string>();
	cfg.slurm_partition = vm["slurm-partition"].as<std::string>();
	{
		std::string slurm_license = vm["slurm-license"].as<std::string>();
		if (slurm_license.size() > 0) {
			cfg.slurm_license = slurm_license;
		}
	}
	cfg.architecture = vm["architecture"].as<std::string>();

	if (auto loglevel_from_env = hxcomm::get_loglevel_from_env("QUIGGELDY_LOGLEVEL");
	    loglevel_from_env) {
		cfg.log_level = *loglevel_from_env;
	}
	logger_default_config(Logger::log4cxx_level_v2(cfg.log_level));
	auto log = log4cxx::Logger::getLogger("hxcomm.quiggeldy");

	if (cfg.hxcomm_log_threshold_at_compile > cfg.log_level) {
		HXCOMM_LOG_WARN(
		    log, "Loglevel was specified as "
		             << cfg.log_level << " but quiggeldy was compiled to only include log levels "
		             << cfg.hxcomm_log_threshold_at_compile
		             << " and above. Messages of lower log levels will NOT be printed!");
	}

	if (cfg.architecture != "vx") {
		HXCOMM_LOG_ERROR(log, "Currently, only vx-architecture is supported!");
		return 1;
	}

	if (!cfg.ignore_parent_death) {
		/* inform child of dying parents */
		prctl(PR_SET_PDEATHSIG, SIGTERM);
	}

	HXCOMM_LOG_INFO(log, "Starting up..");

	// has to be called prior to QuiggeldyServer
	auto thread_sig = quiggeldy::setup_signal_handler_thread();

	// RCF::init() needs to be performed AFTER we have set up signal handling
	// so all threads spawned by RCF will honor our signal masks
	RCF::init();

	std::unique_ptr<quiggeldy_server_t> server;

	// create server
	if (cfg.backend_arq + cfg.backend_axi + cfg.backend_sim > 1) {
		std::cerr
		    << "Please specify only one of: --conneciton-arq --connection-axi --connection-sim."
		    << std::endl;
		return EXIT_FAILURE;
	} else if (cfg.backend_arq + cfg.backend_axi + cfg.backend_sim == 0) {
		cfg.backend_arq = true;
	}

	if (cfg.backend_arq) {
		if (!cfg.mock_mode && cfg.connect_ip.length() == 0) {
			HXCOMM_LOG_ERROR(log, "--connect-ip required for non-mock operation!");
			return 1;
		}
		HXCOMM_LOG_DEBUG(log, "Setting up ARQ-based worker to connect to: " << cfg.connect_ip);
		auto worker = QuiggeldyWorkerARQ(cfg.connect_ip);
		quiggeldy::configure(worker, cfg);
		quiggeldy::allocate<QuiggeldyServerARQ>(server, std::move(worker), cfg);
	} else if (cfg.backend_axi) {
		if (cfg.connect_ip.length() != 0) {
			HXCOMM_LOG_WARN(
			    log, "Specified --connect-ip, but irrelevant for AXI-connection -> ignoring..");
		}
		if (cfg.connect_port != 0) {
			HXCOMM_LOG_WARN(
			    log, "Specified --connect-port, but irrelevant for AXI-connection -> ignoring..");
		}
		HXCOMM_LOG_DEBUG(log, "Setting up AXI-based worker to connect to..");
		HXCOMM_LOG_WARN(
		    log,
		    "AXI-connection only meant to be run locally on zynq, disabling slurm allocation..");
		cfg.no_allocate_license = true;
		auto worker = QuiggeldyWorkerAXI();
		quiggeldy::configure(worker, cfg);
		quiggeldy::allocate<QuiggeldyServerAXI>(server, std::move(worker), cfg);
	} else if (cfg.backend_sim) {
		HXCOMM_LOG_DEBUG(
		    log, "Setting up CoSim-based worker to connect to: " << cfg.connect_ip << ":"
		                                                         << cfg.connect_port);
		auto worker = QuiggeldyWorkerSim(cfg.connect_ip, cfg.connect_port);
		quiggeldy::configure(worker, cfg);
		quiggeldy::allocate<QuiggeldyServerSim>(server, std::move(worker), cfg);
	}

	bool work_left_at_shutdown = false;
	// we want to release a possible slurm allocation if the program fails under any circumstances
	quiggeldy::signal_handler = [&server, &work_left_at_shutdown, &log](int sig) {
		auto log = log4cxx::Logger::getLogger("hxcomm.quiggeldy.signal_handler");
		if (!server) {
			HXCOMM_LOG_TRACE(log, "Server already terminated, exiting signal handler..");
			return 1;
		}

		HXCOMM_LOG_DEBUG(log, "Received signal: " << sig << " (" << strsignal(sig) << ").");
		auto shutdown = [&log, &server, &work_left_at_shutdown] {
			work_left_at_shutdown =
			    std::visit([](auto const& server) { return server.has_work_left(); }, *server);
			HXCOMM_LOG_DEBUG(log, "Shutting down server.");
			server.reset();
		};
		switch (sig) {
			case SIGCONT: // continue signal with which we indicate a reset of the idle timeout
				HXCOMM_LOG_DEBUG(log, "Resetting idle timeout!");
				std::visit([](auto&& srv) { srv.reset_idle_timeout(); }, *server);
				return 0;
			case SIGABRT: // abnormal termination condition, as is e.g. initiated by std::abort()
			case SIGFPE:  // erroneous arithmetic operation such as divide by zero
			case SIGILL:  // invalid program image, such as invalid instruction
			case SIGINT:  // external interrupt, usually initiated by the user
			case SIGSEGV: // invalid memory access (segmentation fault)
			case SIGTERM: // termination request, sent to the program
			case SIGUSR2: // shutdown-signal also used in sctrltp
			case SIGHUP:  // controlling terminal lost
				shutdown();
				return 1;
			default:
				HXCOMM_LOG_DEBUG(log, "Ignoring singal..");
				return 0;
		}
	};

	bool timeout_reached = std::visit(
	    [&cfg, &log](auto&& srv) -> bool {
		    // Set max message length to the same amount as in client (same for all
		    // instantiations)
		    HXCOMM_LOG_TRACE(log, "Setting release interval to " << cfg.release_seconds << "s.");
		    srv.set_release_interval(std::chrono::seconds(cfg.release_seconds));
		    srv.get_server().getServerTransport().setMaxIncomingMessageLength(
		        hxcomm::quiggeldy_max_message_length);

		    HXCOMM_LOG_INFO(log, "Quiggeldy set up!");
		    return srv.start_server(std::chrono::seconds(cfg.timeout_seconds));
	    },
	    *server);

	if (timeout_reached) {
		HXCOMM_LOG_INFO(log, "Quiggeldy shutting down due to idle timeout.");
	}
	server.reset();
	HXCOMM_LOG_TRACE(log, "Joining signal handler..");
	if (pthread_kill(thread_sig, SIGCONT) != 0) // join signal handling thread
	{
		HXCOMM_LOG_FATAL(log, "Could not send signal to signal-handler thread.");
	}
	if (pthread_join(thread_sig, NULL) != 0) {
		HXCOMM_LOG_FATAL(log, "Could not join signal handling thread.");
	}
	RCF::deinit();
	if (work_left_at_shutdown) {
		HXCOMM_LOG_ERROR(log, "There was work left on scheduler shutdown.");
		return 1;
	} else {
		return 0;
	}
}
