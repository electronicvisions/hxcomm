#include "hxcomm/vx/axiconnection.h"

#include <iostream>
#include <queue>
#include <vector>

#include <gtest/gtest.h>

TEST(TestAXIConnection, AXILoopback)
{
	using namespace hxcomm::vx;
	using namespace hxcomm::vx::instruction;

	hxcomm::AXIHandle axi_handle;

	std::queue<uint64_t> q;
	q.push(0x1234);

	axi_handle.send(q);

	auto rets = axi_handle.receive();

	for (auto const& r : rets) {
		std::cout << std::hex << r << std::endl;
	}
}
