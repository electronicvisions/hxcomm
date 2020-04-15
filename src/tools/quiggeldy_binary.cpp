#include <functional>
#include <iostream>
#include <utility>
#include <variant>
#include <boost/program_options.hpp>

#include "pthread.h"
#include "signal.h"

#include "logger.h"
#include "logging_ctrl.h"

#include "hxcomm/common/cerealization_utmessage.h"
#include "hxcomm/vx/arqconnection.h"
#include "hxcomm/vx/simconnection.h"

#include "hxcomm/common/logger.h"

// include for each target
#include "hxcomm/vx/detail/quiggeldy_server.h"

// needed to have lambdas as signal handlers
namespace quiggeldy {

std::function<int(int)> signal_handler;

void* handle_signals_thread(void*);

pthread_t setup_signal_handler_thread()
{
	auto log = log4cxx::Logger::getLogger(std::string("quiggeldy.") + __func__);
	HXCOMM_LOG_TRACE(log, "Setting up signal handler..");

	// have all threads not listen to signals
	sigset_t sigset;
	pthread_t sigthread;

	// set dummy function that ignores all signals to avoid crashing if a
	// signal is received in the short amount of time until the
	// QuiggeldyServer is running
	quiggeldy::signal_handler = []([[maybe_unused]] int sig) {
		auto log = log4cxx::Logger::getLogger(std::string("quiggeldy.") + __func__);
		HXCOMM_LOG_DEBUG(log, "Received signal " << sig << " in dummy signal handler, ignoring..");
		return 0;
	};

	sigfillset(&sigset);
	pthread_sigmask(SIG_BLOCK, &sigset, NULL);
	pthread_create(&sigthread, NULL, handle_signals_thread, NULL);

	HXCOMM_LOG_TRACE(log, "Signal handling set up.");

	return sigthread;
}

void* handle_signals_thread(void*)
{
	auto log = log4cxx::Logger::getLogger(std::string("quiggeldy.") + __func__);

	HXCOMM_LOG_TRACE(log, "Setting up signal handler thread");

	int sig;
	sigset_t sigset;

	sigemptyset(&sigset);

	// termination request, sent to the program
	sigaddset(&sigset, SIGTERM);

	// invalid memory access (segmentation fault)
	sigaddset(&sigset, SIGSEGV);

	// external interrupt, usually initiated by the user
	sigaddset(&sigset, SIGINT);

	// invalid program image, such as invalid instruction
	sigaddset(&sigset, SIGILL);

	// abnormal termination condition, as is e.g. initiated by std::abort()
	sigaddset(&sigset, SIGABRT);

	// erroneous arithmetic operation such as divide by zero
	sigaddset(&sigset, SIGFPE);

	// continue signal with which we indicate a reset of the idle timeout
	sigaddset(&sigset, SIGCONT);

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

	uint32_t release_seconds;
	uint32_t timeout_seconds;

	size_t log_level;
	size_t num_threads_input;
	size_t num_threads_output;

	bool mock_mode;
	bool no_allocate_gres;
	bool no_munge;

	std::string architecture;
	std::string slurm_partition;

	bool backend_arq;
	bool backend_sim;

	static size_t hxcomm_log_threshold_at_compile;
};

#ifdef HXCOMM_LOG_THRESHOLD
size_t Config::hxcomm_log_threshold_at_compile = HXCOMM_LOG_THRESHOLD;
#else
size_t Config::hxcomm_log_threshold_at_compile = 0;
#endif

template <class WorkerT>
void configure(WorkerT& worker, Config& cfg)
{
	auto log = log4cxx::Logger::getLogger("quiggeldy.configure");

	if (cfg.mock_mode) {
		HXCOMM_LOG_INFO(log, "Setting mock-mode.");
	}
	worker.set_enable_mock_mode(cfg.mock_mode);

	if (cfg.no_allocate_gres) {
		HXCOMM_LOG_INFO(
		    log, "NOT allocating gres prior to executing work! This should only be enabled for "
		         "testing and debugging!")
	}
	worker.set_enable_allocate_gres(!cfg.no_allocate_gres);
	HXCOMM_LOG_DEBUG(log, "Setting slurm partition: " << cfg.slurm_partition);
	worker.set_slurm_partition(cfg.slurm_partition);

	if (cfg.no_munge) {
		HXCOMM_LOG_DEBUG(log, "Excplicitly disabling munge authentication.")
		worker.set_use_munge(false);
	}
}

template <class ServerT, class WorkerT, class VariantT>
void allocate(std::unique_ptr<VariantT>& server, WorkerT&& worker, Config& cfg)
{
	server.reset(new VariantT{std::in_place_type<ServerT>,
	                          RCF::TcpEndpoint(cfg.listen_ip, cfg.listen_port), std::move(worker),
	                          cfg.num_threads_input, cfg.num_threads_output});
}

} // namespace quiggeldy

namespace po = boost::program_options;

using QuiggeldyWorkerARQ = hxcomm::vx::detail::QuiggeldyWorker<hxcomm::vx::ARQConnection>;
using QuiggeldyWorkerSim = hxcomm::vx::detail::QuiggeldyWorker<hxcomm::vx::SimConnection>;

using QuiggeldyServerARQ = hxcomm::vx::detail::QuiggeldyServer<hxcomm::vx::ARQConnection>;
using QuiggeldyServerSim = hxcomm::vx::detail::QuiggeldyServer<hxcomm::vx::SimConnection>;

using quick_queue_server_t = std::variant<QuiggeldyServerARQ, QuiggeldyServerSim>;

int main(int argc, const char* argv[])
{
	quiggeldy::Config cfg;

	po::options_description desc("Allowed options");
	desc.add_options()("help,h", "produce help message")(
	    "listen-ip", po::value<std::string>()->default_value("0.0.0.0"), "specify listening IP")(
	    "listen-port,p", po::value<uint16_t>(&(cfg.listen_port))->required(),
	    "specify listening port")(
	    "connect-ip,i", po::value<std::string>()->default_value(""), "IP to connect to")(
	    "connect-port", po::value<uint16_t>(&(cfg.connect_port)),
	    "Port to connect to (if --sim-connection specified)")(
	    "release,r", po::value<uint32_t>(&(cfg.release_seconds))->default_value(600),
	    "Number of seconds between releases of slurm allocation")(
	    "architecture,a", po::value<std::string>()->default_value("vx"),
	    "Which architecture to target? Currently there is only vx supported.")(
	    "slurm-partition", po::value<std::string>()->default_value("cube"),
	    "Slurm partition in which to allocate gres (if not disabled via --no-allocate-gres).")(
	    "timeout,t", po::value<uint32_t>(&(cfg.timeout_seconds))->default_value(0),
	    "Number of seconds after which quiggeldy shuts down when idling (0=disable).")(
	    "loglevel,l", po::value<size_t>(&(cfg.log_level))->default_value(1),
	    "specify loglevel [0-ERROR,1-WARNING,2-INFO,3-DEBUG,4-TRACE]")(
	    "num-threads-input,n", po::value<size_t>(&(cfg.num_threads_input))->default_value(8),
	    "Number of threads handling incoming connections.")(
	    "num-threads-outputs,m", po::value<size_t>(&(cfg.num_threads_output))->default_value(8),
	    "Number of threads handling distribution of results.")(
	    "mock-mode", po::bool_switch(&(cfg.mock_mode))->default_value(false),
	    "Operate in mock-mode, i.e., accept connections but return empty results.")(
	    "arq-connection", po::bool_switch(&(cfg.backend_arq))->default_value(false),
	    "Proxy an ARQ-connection (default mode of operation).")(
	    "sim-connection", po::bool_switch(&(cfg.backend_sim))->default_value(false),
	    "Proxy connection to simulation backend.")(
	    "no-allocate-gres", po::bool_switch(&(cfg.no_allocate_gres))->default_value(false),
	    "Do not allocate gres prior to running jobs.")(
	    "no-munge", po::bool_switch(&(cfg.no_munge))->default_value(false),
	    "Do not verify clients using munge.");

	// populate vm variable
	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);

	if (vm.count("help")) {
		std::cerr << desc << std::endl;
		return EXIT_SUCCESS;
	}
	po::notify(vm);

	// extract all string variables from vm because supplying references to
	// them when creating the argument leads to data corruption
	cfg.listen_ip = vm["listen-ip"].as<std::string>();
	cfg.connect_ip = vm["connect-ip"].as<std::string>();
	cfg.slurm_partition = vm["slurm-partition"].as<std::string>();
	cfg.architecture = vm["architecture"].as<std::string>();

	logger_default_config(Logger::log4cxx_level(cfg.log_level));
	auto log = log4cxx::Logger::getLogger("quiggeldy");

	if (cfg.hxcomm_log_threshold_at_compile < cfg.log_level) {
		HXCOMM_LOG_WARN(
		    log, "Loglevel was specified as "
		             << cfg.log_level << " but quiggeldy was compiled to only include log levels "
		             << cfg.hxcomm_log_threshold_at_compile
		             << " and below. Messages of higher log levels will NOT be printed!");
	}

	if (cfg.architecture != "vx") {
		HXCOMM_LOG_ERROR(log, "Currently, only vx-architecture is supported!");
		return 1;
	}

	HXCOMM_LOG_INFO(log, "Starting up..");

	// has to be called prior to QuiggeldyServer
	auto sig_thread = quiggeldy::setup_signal_handler_thread();

	std::unique_ptr<quick_queue_server_t> server;

	// create server
	if (cfg.backend_arq + cfg.backend_sim > 1) {
		std::cerr << "Please specify only one of: --arq-conneciton --sim-connection." << std::endl;
		return EXIT_FAILURE;
	} else if (cfg.backend_arq + cfg.backend_sim == 0) {
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

	} else if (cfg.backend_sim) {
		HXCOMM_LOG_DEBUG(
		    log, "Setting up CoSim-based worker to connect to: " << cfg.connect_ip << ":"
		                                                         << cfg.connect_port);
		auto worker = QuiggeldyWorkerSim(cfg.connect_ip, cfg.connect_port);
		quiggeldy::configure(worker, cfg);
		quiggeldy::allocate<QuiggeldyServerSim>(server, std::move(worker), cfg);
	}

	// we want to release a possible slurm allocation if the program fails under any circumstances
	quiggeldy::signal_handler = [&server, &log](int sig) {
		HXCOMM_LOG_DEBUG(log, "Received signal " << sig << " in signal handler..");
		switch (sig) {
			case SIGCONT:
				HXCOMM_LOG_DEBUG(log, "Resetting idle timeout!");
				std::visit([](auto&& srv) { srv.reset_idle_timeout(); }, *server);
				return 0;
			default:
				HXCOMM_LOG_DEBUG(log, "Shutting down server!");
				std::visit(
				    [](auto&& srv) { srv.shutdown(); },
				    *server); // worker teardown handled in shutdown
				return -1;
		}
	};

	// Set max message length to the same amount as in client (same for all instantiations)
	std::visit(
	    [&cfg, &log](auto&& srv) {
		    srv.set_release_interval(std::chrono::seconds(cfg.release_seconds));
		    srv.get_server().getServerTransport().setMaxIncomingMessageLength(
		        hxcomm::quiggeldy_max_message_length);

		    HXCOMM_LOG_INFO(log, "Quiggeldy set up!");
		    srv.start_server(std::chrono::seconds(cfg.timeout_seconds));
	    },
	    *server);
	HXCOMM_LOG_INFO(log, "Quiggeldy shutting down due to idle timeout.");

	// cancel sighandler thread if it is still runningj
	pthread_cancel(sig_thread);
	return 0;
}
