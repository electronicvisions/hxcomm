#include <gtest/gtest.h>

#include <optional>
#include <tuple>

#include "hxcomm/common/connection_context.h"

struct RefCounter
{
	static int count;

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

	using init_parameters_type = std::tuple<int, double>;

	std::optional<int> m_fancy;
	std::optional<double> m_params;
};


int RefCounter::count = 0;


TEST(ConnectionContext, Simple)
{
	using context_type = hxcomm::Context<RefCounter>;
	ASSERT_EQ(RefCounter::count, 0);

	context_type context;
	ASSERT_EQ(RefCounter::count, 0);

	auto proxy = context.__enter__();
	ASSERT_EQ(RefCounter::count, 1);

	context.__exit__();
	ASSERT_EQ(RefCounter::count, 0);
    ASSERT_FALSE(proxy->get());
}

TEST(ConnectionContext, DoubleAllocation)
{
	using context_type = hxcomm::Context<RefCounter>;
	ASSERT_EQ(RefCounter::count, 0);

	context_type context;
	ASSERT_EQ(RefCounter::count, 0);

	{
		auto proxy = context.__enter__();
		ASSERT_EQ(RefCounter::count, 1);
		ASSERT_TRUE(proxy.get());
	}
	{
		auto proxy = context.__enter__();
		ASSERT_EQ(RefCounter::count, 1);
		ASSERT_TRUE(proxy.get());
	}

	context.__exit__();
	ASSERT_EQ(RefCounter::count, 0);
}

TEST(ConnectionContext, WithInitParams)
{
	using context_type = hxcomm::Context<RefCounter>;
	ASSERT_EQ(RefCounter::count, 0);

	context_type context(42, 6.8);
	ASSERT_EQ(RefCounter::count, 0);

	auto proxy = context.__enter__();
	ASSERT_EQ(RefCounter::count, 1);

	{
		auto conn_opt = proxy->get();
        ASSERT_TRUE(conn_opt);
        auto conn = *conn_opt;
		ASSERT_EQ(conn->m_fancy, 42);
		ASSERT_EQ(conn->m_params, 6.8);
	}

	context.__exit__();
	ASSERT_EQ(RefCounter::count, 0);
	ASSERT_FALSE(proxy.get());
}
