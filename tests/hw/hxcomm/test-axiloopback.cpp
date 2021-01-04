#include "hxcomm/vx/axiconnection.h"

#include <iostream>
#include <queue>
#include <vector>

#include <gtest/gtest.h>

TEST(TestAXIConnection, AXILoopback)
{
	using namespace hxcomm::vx;
	using namespace hxcomm::vx::instruction;

	hxcomm::AXIHandle axi_handle(0xa001'0000, 0xa002'0000, 0xa003'0000, 0xa004'0000);

	std::queue<uint64_t> q;
	q.push(0x1234);

	axi_handle.send(q);

	auto rets = axi_handle.receive();

	for (auto const& r : rets) {
		std::cout << std::hex << r << std::endl;
	}
}

TEST(TestAXIConnection, ExtMemAccess)
{
	hxcomm::vx::FrickelExtMem fem;


	{
		std::vector<uint32_t> tmp(2);
		fem.write(0x0, tmp);
		auto read_data = fem.read<std::vector<uint32_t>>(0x0, 2);
		EXPECT_EQ(read_data, tmp);
	}

	{
		auto [begin, end] = fem.get_iterators<uint32_t>();
		EXPECT_GT(end-begin, 4);
		*begin = 17+4;
		*(begin+1) = 0;
		auto read_data = fem.read<std::vector<uint32_t>>(0x0, 2);
		EXPECT_EQ(read_data.size(), 2);
		EXPECT_EQ(read_data[0], 17+4);
		EXPECT_EQ(read_data[1], 0);

		std::fill(begin, end, 0);
		EXPECT_EQ(*begin, 0);
		EXPECT_EQ(*(end-1), 0);
	}
}
