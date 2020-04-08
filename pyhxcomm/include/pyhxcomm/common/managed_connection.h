#pragma once

#include "hate/visibility.h"
#include "hxcomm/common/execute_messages.h"
#include "hxcomm/common/logger.h"
#include "pyhxcomm/common/handle_connection.h"
#include "pyhxcomm/common/visit_connection.h"

#include <memory>
#include <optional>
#include <type_traits>
#include <boost/hana/string.hpp>
#include <pybind11/pybind11.h>

namespace log4cxx {

class Logger;

} // namespace log4cxx

namespace pyhxcomm {

namespace detail {

template <typename Connection, typename = void>
struct init_parameters
{
	using type = std::tuple<>;
};

template <typename Connection>
struct init_parameters<Connection, std::void_t<typename Connection::init_parameters_type>>
{
	using type = typename Connection::init_parameters_type;
};

/**
 * Helper function that creates a variant over handles of all connections.
 */
template <typename T>
struct add_handle
{};

template <typename... Ts>
struct add_handle<std::variant<Ts...>>
{
	using type = std::variant<pyhxcomm::Handle<Ts>...>;
};

} // namespace detail

/**
 * Context manager to port RAII-style of connection into the python world.
 *
 * Ensures that the connection held by the returned Handle-object upon
 * enter()-ing the managed gets destroyed upon exit()-ing the managed.
 */
template <typename Connection>
class Managed
{
public:
	using connection_type = Connection;
	using connection_init_parameters_type = typename detail::init_parameters<connection_type>::type;
	using handle_type = Handle<connection_type>;
	using shared_handle_type = std::shared_ptr<handle_type>;
	using weak_handle_type = std::weak_ptr<handle_type>;

	/**
	 * Create a managed that will construct the underlying connection with given arguments upon
	 * entering.
	 *
	 * Upon exiting, the handle object will be deleted.
	 *
	 * @tparam Args Arguments given to connection.
	 */
	template <typename... Args>
	Managed(Args&&...);

	/**
	 * No copy as one Managed governs one connection.
	 */
	Managed(Managed const&) = delete;
	Managed& operator=(Managed const&) = delete;

	/**
	 * Move constructor.
	 */
	Managed(Managed&&);

	/**
	 * Enter the managed with allocated connection.
	 *
	 * @return Handle-object handling the underlying connection.
	 */
	shared_handle_type enter();

	/**
	 * Destroys the connection in the current handle object.
	 */
	void exit();

private:
	void disconnect();

	std::optional<connection_init_parameters_type> m_init_params;

	weak_handle_type m_handle;

	void setup_logger();

	log4cxx::Logger* m_logger;
};

namespace detail {

template <typename T>
struct HandleVariantConstructor
{
	static_assert(sizeof(T) == 0, "Only useful for variants.");
};

/**
 * Helper struct that is needed to specify how a given connection variant is constructed.
 *
 * Needs to be specified in architecture specific code.
 */
template <typename... Ts>
struct HandleVariantConstructor<std::variant<Ts...>>
{
	static_assert(sizeof(std::variant<Ts...>) == 0, "Do not know how to construct this variant.");

	static std::variant<std::shared_ptr<Ts>...> construct();
};

} // namespace detail

template <typename... Connections>
class Managed<std::variant<Connections...>>
{
public:
	using variant_type = std::variant<Connections...>;
	using handle_type = std::variant<Handle<Connections>...>;
	using shared_handle_type = std::variant<std::shared_ptr<Handle<Connections>>...>;
	using weak_handle_type = std::variant<std::weak_ptr<Handle<Connections>>...>;
	using handle_constructor = detail::HandleVariantConstructor<variant_type>;

	/**
	 * Default construct a variant.
	 *
	 * Upon exiting, the handle object will be deleted.
	 *
	 * @tparam Args Arguments given to connection.
	 */
	Managed();

	/**
	 * No copy as one Managed governs one connection.
	 */
	Managed(Managed const&) = delete;
	Managed& operator=(Managed const&) = delete;

	/**
	 * Move constructor.
	 */
	Managed(Managed&&);

	/**
	 * Enter the managed with allocated connection.
	 *
	 * @return Handle-object handling the underlying connection.
	 */
	shared_handle_type enter();

	/**
	 * Destroys the connection in the current handle object.
	 */
	void exit();

private:
	void disconnect();

	weak_handle_type m_handle;

	void setup_logger();

	log4cxx::Logger* m_logger;
};

/**
 * Helper class that only create the pybind11-bindings for the Managed-type but
 * not the underlying handle.
 *
 * Only useful if the enter()-method does not return a strict handler but, for
 * example, a variant over handlers.
 */
template <typename Connection>
struct SYMBOL_VISIBLE ManagedOnlyPyBind11Helper
{
	/**
	 * Create binding.
	 *
	 * @param parent pybind11::module in which the mapping should be created.
	 * @param name Which name should be given to the context manager in python.
	 */
	template <typename HanaString>
	constexpr ManagedOnlyPyBind11Helper(pybind11::module& parent, HanaString const& name);

	using connection_type = Connection;
	using managed_type = Managed<connection_type>;

	using pb11_managed_type = pybind11::class_<managed_type>;

	pb11_managed_type managed;
};

/**
 * Helper class to generate pybind11 bindings for context managers.
 */
template <typename Connection>
struct SYMBOL_VISIBLE ManagedPyBind11Helper : ManagedOnlyPyBind11Helper<Connection>
{
	/**
	 * Create binding.
	 *
	 * @param parent pybind11::module in which the mapping should be created.
	 * @param name Which name should be given to the context manager in python.
	 */
	template <typename HanaString>
	constexpr ManagedPyBind11Helper(pybind11::module& parent, HanaString const& name);

	using connection_type = Connection;
	using parent_type = ManagedOnlyPyBind11Helper<connection_type>;
	using managed_type = typename parent_type::managed_type;
	using handle_type = typename managed_type::handle_type;

	using pb11_handle_type = pybind11::class_<handle_type, std::shared_ptr<handle_type>>;

	pb11_handle_type handle;
};

} // namespace pyhxcomm

#include "pyhxcomm/common/managed_connection.tcc"
