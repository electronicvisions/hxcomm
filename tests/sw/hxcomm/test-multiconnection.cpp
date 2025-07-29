#include "gtest/gtest.h"

#include "hxcomm/vx/connection_from_env.h"
#include <cstdlib>

TEST(Multiconnection, Size)
{
	using namespace hxcomm;

	setenv("HXCOMM_ENABLE_ZERO_MOCK", "1", 1);

	vx::ConnectionVariant variant = vx::get_connection_from_env();

	size_t size = std::visit([](auto& connection) { return connection.size(); }, variant);

	EXPECT_EQ(
	    std::visit([](auto& connection) { return connection.get_time_info(); }, variant).size(),
	    size);

	EXPECT_EQ(
	    std::visit([](auto& connection) { return connection.get_unique_identifier(); }, variant)
	        .size(),
	    size);

	EXPECT_EQ(
	    std::visit([](auto& connection) { return connection.get_bitfile_info(); }, variant).size(),
	    size);

	EXPECT_EQ(
	    std::visit([](auto& connection) { return connection.get_remote_repo_state(); }, variant)
	        .size(),
	    size);

	EXPECT_EQ(
	    std::visit([](auto& connection) { return connection.get_hwdb_entry(); }, variant).size(),
	    size);

	auto connection = std::move(std::get<vx::MultiZeroMockConnection>(variant));
	for (size_t i = 0; i < size; i++) {
		EXPECT_NO_THROW(connection[i]);
	}
}
