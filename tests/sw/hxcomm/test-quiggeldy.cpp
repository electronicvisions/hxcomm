#include "gtest/gtest.h"

#include "hxcomm/common/logger.h"
#include "hxcomm/common/quiggeldy_utility.h"
#include "hxcomm/common/stream_rc.h"
#include "hxcomm/vx/quiggeldy_connection.h"

#include <charconv>

#include <sys/types.h>
#include <sys/wait.h>


TEST(Quiggeldy, SimpleMockModeSynchronous)
{
	using namespace hxcomm;

	auto log = log4cxx::Logger::getLogger("TestQuiggeldy");
	HXCOMM_LOG_TRACE(log, "Starting");
	int status;

	hxcomm::port_t port = get_unused_port();

	int quiggeldy_pid = setup_quiggeldy(
	    "quiggeldy", port, "--mock-mode", "--timeout", "10",
	    hxcomm::is_munge_available() ? "" : "--no-munge");

	auto client = hxcomm::vx::QuiggeldyConnection("127.0.0.1", port);
	StreamRC<decltype(client)> stream{client};

	// synchronous call

	auto response = stream.submit_blocking(decltype(client)::interface_types::request_type());
	ASSERT_EQ(response.first.size(), 0);

	HXCOMM_LOG_TRACE(log, "Killing quiggeldy.");
	kill(quiggeldy_pid, SIGTERM);
	HXCOMM_LOG_TRACE(log, "Waiting for quiggeldy to terminate.");
	waitpid(quiggeldy_pid, &status, 0); // wait for the child to exit
	ASSERT_TRUE(WIFEXITED(status));
	ASSERT_EQ(WEXITSTATUS(status), 0);
}

TEST(Quiggeldy, SimpleMockModeAsynchronous)
{
	using namespace hxcomm;

	auto log = log4cxx::Logger::getLogger("TestQuiggeldy");
	HXCOMM_LOG_TRACE(log, "Starting");

	hxcomm::port_t port = get_unused_port();

	int quiggeldy_pid = setup_quiggeldy(
	    "quiggeldy", port, "--mock-mode", "--timeout", "10",
	    hxcomm::is_munge_available() ? "" : "--no-munge");

	auto client = hxcomm::vx::QuiggeldyConnection("127.0.0.1", port);
	StreamRC<decltype(client)> stream_rc{client};

	// asynchronous calls
	std::vector<typename decltype(client)::future_type> futures;
	for (std::size_t i = 0; i < 100; ++i) {
		futures.push_back(
		    stream_rc.submit_async(decltype(client)::interface_types::request_type()));
	}

	std::size_t idx = 0;
	for (auto& future : futures) {
		HXCOMM_LOG_TRACE(log, "Asserting future #" << idx);
		ASSERT_EQ((*future).first.size(), 0);
		++idx;
	}

	HXCOMM_LOG_TRACE(log, "Killing quiggeldy.");
	kill(quiggeldy_pid, SIGTERM);
	HXCOMM_LOG_TRACE(log, "Waiting for quiggeldy to terminate.");
	int status;
	waitpid(quiggeldy_pid, &status, 0); // wait for the child to exit
	ASSERT_TRUE(WIFEXITED(status));
	ASSERT_EQ(WEXITSTATUS(status), 0);
}

TEST(Quiggeldy, SimpleMockModeReinit)
{
	using namespace hxcomm;

	auto log = log4cxx::Logger::getLogger("TestQuiggeldy");
	HXCOMM_LOG_TRACE(log, "Starting");

	hxcomm::port_t port = get_unused_port();

	int quiggeldy_pid = setup_quiggeldy(
	    "quiggeldy", port, "--mock-mode", "--timeout", "10",
	    hxcomm::is_munge_available() ? "" : "--no-munge");

	auto client1 = hxcomm::vx::QuiggeldyConnection("127.0.0.1", port);
	auto client2 = hxcomm::vx::QuiggeldyConnection("127.0.0.1", port);
	StreamRC<decltype(client1)> stream1_rc{client2};
	StreamRC<decltype(client2)> stream2_rc{client2};

	stream1_rc.register_reinit(typename decltype(client1)::interface_types::reinit_type());
	/* no reinit for stream 2 because no reinit program should also work */

	// asynchronous calls
	std::vector<typename decltype(client1)::future_type> futures;
	for (std::size_t i = 0; i < 100; ++i) {
		futures.push_back(
		    stream1_rc.submit_async(decltype(client1)::interface_types::request_type()));
		futures.push_back(
		    stream2_rc.submit_async(decltype(client2)::interface_types::request_type()));
	}

	std::size_t idx = 0;
	for (auto& future : futures) {
		HXCOMM_LOG_TRACE(log, "Asserting future #" << idx);
		ASSERT_EQ((*future).first.size(), 0);
		++idx;
	}

	HXCOMM_LOG_TRACE(log, "Killing quiggeldy.");
	kill(quiggeldy_pid, SIGTERM);
	HXCOMM_LOG_TRACE(log, "Waiting for quiggeldy to terminate.");
	int status;
	waitpid(quiggeldy_pid, &status, 0); // wait for the child to exit
	ASSERT_TRUE(WIFEXITED(status));
	ASSERT_EQ(WEXITSTATUS(status), 0);
}
