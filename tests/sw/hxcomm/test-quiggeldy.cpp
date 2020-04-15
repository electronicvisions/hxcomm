#include "gtest/gtest.h"

#include "hxcomm/common/logger.h"
#include "hxcomm/common/quiggeldy_utility.h"
#include "hxcomm/common/stream.h"
#include "hxcomm/vx/quiggeldy_client.h"

#include <sys/types.h>
#include <sys/wait.h>


TEST(Quiggeldy, SimpleMockMode)
{
	using namespace hxcomm;

	int quiggeldy_pid, status;

	auto port = get_unused_port();
	ASSERT_GT(port, 0);

	{
		quiggeldy_pid = setup_quiggeldy(
		    "quiggeldy", port, "--mock-mode", "--timeout", "10",
		    hxcomm::is_munge_available() ? "" : "--no-munge");
		ASSERT_GT(quiggeldy_pid, 0);
	}

	auto client = hxcomm::vx::QuiggeldyClient("127.0.0.1", uint16_t(port));

	Stream<decltype(client)> stream(client);
	auto response = stream.submit_blocking(decltype(client)::quiggeldy_request_type());
	ASSERT_EQ(response.size(), 0);

	kill(quiggeldy_pid, SIGTERM);
	waitpid(quiggeldy_pid, &status, 0); // wait for the child to exit
	ASSERT_TRUE(WIFSIGNALED(status));
	ASSERT_EQ(WTERMSIG(status), SIGTERM);
	ASSERT_EQ(WEXITSTATUS(status), 0);
}
