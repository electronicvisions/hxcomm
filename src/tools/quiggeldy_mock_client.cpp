// Simple mock client connector to debug possible errors.

#include "hxcomm/common/logger.h"
#include "hxcomm/common/quiggeldy_utility.h"
#include "hxcomm/common/stream_rc.h"
#include "hxcomm/vx/quiggeldy_connection.h"

#include <iostream>

int main(int argc, char* argv[])
{
	logger_default_config(Logger::log4cxx_level_v2(HXCOMM_LOG_THRESHOLD));
	auto log = log4cxx::Logger::getLogger("hxcomm.quiggeldy_mock_client");

	hxcomm::port_t port;
	if (argc < 2) {
		HXCOMM_LOG_ERROR(log, "Usage: " << argv[0] << " <port>");
		return 1;
	} else {
		port = atoi(argv[1]);
	}

	using namespace hxcomm;

	auto client = hxcomm::vx::QuiggeldyConnection("127.0.0.1", port);

	hxcomm::StreamRC<decltype(client)> stream_rc{client};
	auto response = stream_rc.submit_blocking(decltype(client)::interface_types::request_type());
	if (response.first.size() == 0) {
		return 0;
	} else {
		return 1;
	}
}
