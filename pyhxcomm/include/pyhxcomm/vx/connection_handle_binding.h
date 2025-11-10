#pragma once

#include "pyhxcomm/common/managed_connection.h"
#include "pyhxcomm/vx/connection_handle.h"

#include "hxcomm/vx/connection_from_env.h"
#include "hxcomm/vx/connection_parameter.h"
#include "hxcomm/vx/connection_variant.h"

#include <boost/hana/string.hpp>

#include <variant>

namespace pyhxcomm {

using namespace boost::hana::literals;

namespace detail {
template <typename>
struct add_shared
{};

template <typename... Ts>
struct add_shared<std::variant<Ts...>>
{
	using type = std::variant<std::shared_ptr<Ts>...>;
};

template <>
struct HandleVariantConstructor<hxcomm::vx::ConnectionVariant>
{
	using variant_handle_type = typename add_shared<vx::ConnectionHandle>::type;

	static variant_handle_type construct()
	{
		return std::visit(
		    [](auto&& conn) -> variant_handle_type {
			    using connection_type = std::decay_t<decltype(conn)>;
			    using single_handle_type = pyhxcomm::Handle<connection_type>;
			    using shared_single_handle_type = std::shared_ptr<single_handle_type>;

			    shared_single_handle_type single =
			        std::make_shared<single_handle_type>(std::move(conn));

			    return variant_handle_type(std::move(single));
		    },
		    hxcomm::vx::get_connection_from_env());
	}
};

} // namespace detail

} // namespace pyhxcomm

GENPYBIND_MANUAL({
	using namespace boost::hana::literals;

	using ConnectionHandle = ::pyhxcomm::vx::ConnectionHandle;

	parent->py::template class_<ConnectionHandle>(parent, "ConnectionHandle");

	auto wrapper = pyhxcomm::ManagedOnlyPyBind11Helper<hxcomm::vx::ConnectionVariant>(
	    parent, BOOST_HANA_STRING("Connection"));
})
