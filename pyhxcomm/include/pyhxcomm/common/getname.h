#pragma once

#include <type_traits>
#include <boost/hana/string.hpp>

namespace pyhxcomm {

namespace hana = boost::hana;

template <typename Connection, typename = std::void_t<>>
struct GetName
{
	static_assert(sizeof(Connection) == 0, "static constexpr char[] Connection::name not defined!");
};

template <typename Connection>
struct GetName<Connection, std::void_t<decltype(Connection::name)>>
{
	static constexpr auto name()
	{
		auto name = hana::integral_constant<char const*, Connection::name>{};
		return hana::to_string(name);
	}
};

} // namespace pyhxcomm
