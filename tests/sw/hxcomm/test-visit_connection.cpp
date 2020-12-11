#include "hxcomm/common/visit_connection.h"
#include <iostream>
#include <type_traits>
#include <variant>
#include <gtest/gtest.h>

struct Foo
{
	int content;
};

struct Bar
{
	int content;
};

TEST(VisitConnection, ConstVariant)
{
	std::variant<Foo, Bar> const my_variant{Foo{42}};

	auto const my_visitor = [](auto&& elem) { return elem.content; };

	EXPECT_EQ(hxcomm::visit_connection(my_visitor, my_variant), 42);
}
