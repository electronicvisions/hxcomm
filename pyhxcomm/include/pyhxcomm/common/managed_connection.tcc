#include "hate/memory.h"
#include "hxcomm/common/execute_messages.h"
#include "hxcomm/common/logger.h"
#include "pyhxcomm/common/getname.h"
#include "pyhxcomm/common/handle_connection.h"
#include "pyhxcomm/common/managed_connection.h"
#include <tuple>
#include <boost/hana/string.hpp>


namespace pyhxcomm {

namespace hana = boost::hana;
using namespace hana::literals;

template <typename Connection>
template <typename... Args>
Managed<Connection>::Managed(Args&&... args)
{
	setup_logger();
	if constexpr (sizeof...(Args) > 0) {
		m_init_params = std::make_tuple(std::forward<Args>(args)...);
	} else {
		m_init_params = std::nullopt;
	}
}

template <typename Connection>
Managed<Connection>::Managed(Managed&& other) :
    m_handle(std::move(other.m_handle)), m_init_params(std::move(other.m_init_params))
{
	setup_logger();
}

template <typename Connection>
void Managed<Connection>::disconnect()
{
	if (!m_handle.expired()) {
		m_handle.lock()->disconnect();
	}
}

template <typename Connection>
typename Managed<Connection>::shared_handle_type Managed<Connection>::enter()
{
	if (!m_handle.expired()) {
		HXCOMM_LOG_WARN(
		    m_logger, "Managed was already allocated! Disconnecting previous allocation..");
		disconnect();
	}

	shared_handle_type shared_handle;
	if (m_init_params) {
		m_handle = shared_handle =
		    hate::memory::make_shared_from_tuple<handle_type>(*m_init_params);
	} else {
		m_handle = shared_handle = shared_handle_type(new handle_type());
	}
	return shared_handle;
}

template <typename Connection>
void Managed<Connection>::setup_logger()
{
	m_logger =
	    log4cxx::Logger::getLogger(("pyhxcomm.Managed"_s + GetName<Connection>::name()).c_str());
}

template <typename Connection>
void Managed<Connection>::exit()
{
	disconnect(); // because we cannot guarantee when the handle is destroyed in python
}

template <typename... Connections>
Managed<std::variant<Connections...>>::Managed()
{
	setup_logger();
}

template <typename... Connections>
Managed<std::variant<Connections...>>::Managed(Managed&& other) :
    m_handle(std::move(other.m_handle))
{
	setup_logger();
}

template <typename... Connections>
typename Managed<std::variant<Connections...>>::shared_handle_type
Managed<std::variant<Connections...>>::enter()
{
	std::visit(
	    [this](auto& handle) {
		    if (!handle.expired()) {
			    HXCOMM_LOG_WARN(
			        this->m_logger,
			        "Managed was already allocated! Disconnecting previous allocation..");
			    this->disconnect();
		    }
	    },
	    m_handle);

	shared_handle_type shared_handle = handle_constructor::construct();

	// get weak pointer to to the shared pointer stored in variant
	m_handle = std::visit(
	    [](auto const& sh_handle) -> weak_handle_type {
		    using shared_ptr_type = std::remove_cvref_t<decltype(sh_handle)>;
		    return weak_handle_type(typename shared_ptr_type::weak_type(sh_handle));
	    },
	    shared_handle);

	return shared_handle;
}

template <typename... Connections>
void Managed<std::variant<Connections...>>::exit()
{
	disconnect(); // because we cannot guarantee when the handle is destroyed in python
}


template <typename... Connections>
void Managed<std::variant<Connections...>>::disconnect()
{
	std::visit(
	    [](auto& handle) {
		    if (!handle.expired()) {
			    handle.lock()->disconnect();
		    }
	    },
	    m_handle);
}

template <typename... Connections>
void Managed<std::variant<Connections...>>::setup_logger()
{
	m_logger = log4cxx::Logger::getLogger("pyhxcomm.ManagedConnection");
}

namespace detail {

template <typename T>
struct init_from_tuple
{
	static_assert(sizeof(T) == 0, "Need to supply tuple!");
};

template <typename... Args>
struct init_from_tuple<std::tuple<Args...>>
{
	static pybind11::detail::initimpl::constructor<Args...> init()
	{
		return {};
	}
};

template <typename C, typename = void>
struct has_init_parameters_type : std::false_type
{};

template <typename C>
struct has_init_parameters_type<C, std::void_t<typename C::init_parameter_type>> : std::true_type
{};

} // namespace detail

template <typename Connection>
template <typename HanaString>
constexpr ManagedOnlyPyBind11Helper<Connection>::ManagedOnlyPyBind11Helper(
    pybind11::module& parent, HanaString const& name) :
    managed(parent, ("Managed"_s + name).c_str())
{
	managed.def(pybind11::init<>())
	    .def("__enter__", &managed_type::enter)
	    .def(
	        "__exit__", [](managed_type& managed, pybind11::object const& /* exc_type */,
	                       pybind11::object const& /* exc_value */,
	                       pybind11::object const& /* traceback */) { managed.exit(); })
	    .def("__exit__", [](managed_type& managed) { managed.exit(); });

	if constexpr (detail::has_init_parameters_type<connection_type>::value) {
		managed.def(
		    detail::init_from_tuple<typename connection_type::init_parameters_type>::init());
	}
}

template <typename Connection>
template <typename HanaString>
constexpr ManagedPyBind11Helper<Connection>::ManagedPyBind11Helper(
    pybind11::module& parent, HanaString const& name) :
    parent_type(parent, name), handle(parent, (name + "Handle"_s).c_str())
{
	handle.def_property_readonly(
	    "time_info", [](handle_type const& h) { return h.get().get_time_info(); });
	handle.def_property_readonly(
	    "bitfile_info", [](handle_type const& h) { return h.get().get_bitfile_info(); });
	handle.def(
	    "get_unique_identifier",
	    [](handle_type const& h, std::optional<std::string> hwdb_path) {
		    return h.get().get_unique_identifier(hwdb_path);
	    },
	    pybind11::arg("hwdb_path") = std::nullopt);
}

} // namespace pyhxcomm
