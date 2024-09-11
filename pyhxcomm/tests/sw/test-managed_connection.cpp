#include "hxcomm/common/execute_messages.h"
#include "pyhxcomm/common/managed_connection.h"
#include "pyhxcomm/common/visit_connection.h"
#include <optional>
#include <tuple>
#include <vector>
#include <gtest/gtest.h>

struct MockMessageTypes
{
	using send_type = int;
	using receive_type = int;
	using send_halt_type = int;
};

struct RefCounter
{
	inline static int count = 0;

	static constexpr char name[] = "RefCounter";

	using message_types = MockMessageTypes;

	RefCounter()
	{
		count++;
	}

	RefCounter(int fancy, double params) : RefCounter()
	{
		m_fancy = fancy;
		m_params = params;
	}

	~RefCounter()
	{
		count--;
	}

	RefCounter(RefCounter const&) = delete;
	RefCounter& operator=(RefCounter const&) = delete;

	using init_parameters_type = std::tuple<int, double>;

	std::optional<int> m_fancy;
	std::optional<double> m_params;
};

namespace hxcomm::detail {

template <>
struct ExecutorMessages<RefCounter>
{
	using connection_type = RefCounter;
	using return_type = execute_messages_return_t<connection_type>;
	using messages_type = execute_messages_argument_t<connection_type>;

	return_type operator()(RefCounter&, messages_type const&, bool const)
	{
		return return_type();
	}
};

} // namespace hxcomm::detail

using managed_type = pyhxcomm::Managed<RefCounter>;

TEST(ConnectionContext, Simple)
{
	ASSERT_EQ(RefCounter::count, 0);

	managed_type managed;
	ASSERT_EQ(RefCounter::count, 0);

	auto handle = managed.enter();
	ASSERT_EQ(RefCounter::count, 1);
	hxcomm::execute_messages(handle, std::vector<int>());

	managed.exit();
	ASSERT_EQ(RefCounter::count, 0);
	EXPECT_THROW(handle->get(), std::runtime_error);
}

TEST(ConnectionContext, DoubleAllocation)
{
	ASSERT_EQ(RefCounter::count, 0);

	managed_type managed;
	ASSERT_EQ(RefCounter::count, 0);

	{
		auto handle = managed.enter();
		ASSERT_EQ(RefCounter::count, 1);
		[[maybe_unused]] auto& conn = handle->get();
		managed.exit();
	}
	ASSERT_EQ(RefCounter::count, 0);
	{
		auto handle = managed.enter();
		ASSERT_EQ(RefCounter::count, 1);
		[[maybe_unused]] auto& conn = handle->get();
		managed.exit();
	}
	// test that calling exit again will not cause an error
	managed.exit();
	ASSERT_EQ(RefCounter::count, 0);
}

TEST(ConnectionContext, WithInitParams)
{
	ASSERT_EQ(RefCounter::count, 0);

	managed_type managed(42, 6.8);
	ASSERT_EQ(RefCounter::count, 0);

	auto handle = managed.enter();
	ASSERT_EQ(RefCounter::count, 1);

	{
		auto& conn = handle->get();
		ASSERT_EQ(conn.m_fancy, 42);
		ASSERT_EQ(conn.m_params, 6.8);

		managed.exit();
		// Accessing conn now will lead to segfault! But since
		// ConnectionProxy<Connection>::get can only be called by friended structs this
		// is fine.
		EXPECT_THROW(handle->get(), std::runtime_error);
		EXPECT_THROW(handle->release(), std::runtime_error);
	}
	{
		handle = managed.enter();
		auto conn = handle->release();
		ASSERT_EQ(conn->m_fancy, 42);
		ASSERT_EQ(conn->m_params, 6.8);

		EXPECT_THROW(handle->get(), std::runtime_error);
		EXPECT_THROW(handle->release(), std::runtime_error);
	}

	managed.exit();
	ASSERT_EQ(RefCounter::count, 0);
	EXPECT_THROW(handle->get(), std::runtime_error);
	EXPECT_THROW(handle->release(), std::runtime_error);
}
