#pragma once

#include "hxcomm/common/connection.h"
#include "hxcomm/common/execute_messages_types.h"

#include <type_traits>
#include <typeinfo>

namespace hxcomm {

template <typename T>
struct PrintMe;

/**
 * StreamRC (pronounced "streamers"), is a class similiar to the Stream-class but
 * with a `R`educed and `C`ustom interface for `R`emote exe`C`ution.
 */
template <typename Connection>
struct StreamRC
{
	using connection_type = Connection;

	using submit_arg_type = detail::execute_messages_argument_t<connection_type>;

	struct Check
	{
		/**
		 * Helper for template engine.
		 */
		template <typename C>
		struct submit_async_return
		{
			using type =
			    decltype(std::declval<C&>().submit_async(std::declval<submit_arg_type const&>()));
		};

		/**
		 * Helper for template engine.
		 */
		template <typename C>
		struct submit_blocking_return
		{
			using type = decltype(
			    std::declval<C&>().submit_blocking(std::declval<submit_arg_type const&>()));
		};

		template <typename, typename = std::void_t<>>
		struct has_method_submit_blocking : std::false_type
		{};

		template <typename C>
		struct has_method_submit_blocking<C, std::void_t<typename submit_blocking_return<C>::type>>
		    : std::true_type
		{};

		template <typename C, typename = std::void_t<>>
		struct has_method_submit_async : std::false_type
		{};

		template <typename C>
		struct has_method_submit_async<C, std::void_t<typename submit_async_return<C>::type>>
		    : std::true_type
		{};

		template <typename C>
		struct has_required_interface
		    : std::conjunction<has_method_submit_async<C>, has_method_submit_blocking<C>>
		{};

		template <typename C, typename = std::void_t<>>
		struct has_method_register_reinit : std::false_type
		{};

		template <typename C>
		struct has_method_register_reinit<
		    C,
		    std::void_t<decltype(std::declval<C&>().register_reinit(
		        std::declval<typename C::interface_types::reinit_type>()))>> : std::true_type
		{};

		template <typename C, typename = std::void_t<>>
		struct has_method_get_uploader_reinit : std::false_type
		{};

		template <typename C>
		struct has_method_get_uploader_reinit<
		    C,
		    std::void_t<decltype(std::declval<C&>().get_uploader_reinit())>> : std::true_type
		{};
	};

	static_assert(
	    Check::template has_required_interface<connection_type>::value,
	    "Connection does not adhere to required StreamRC-Interface.");
	static_assert(
	    Check::template has_method_submit_async<connection_type>::value,
	    "Connection does not have submit_async-method");
	static_assert(
	    Check::template has_method_submit_blocking<connection_type>::value,
	    "Connection does not have submit_blocking-method");

	/**
	 * Construct a Stream for the given connection handle which it manages.
	 * Lock connection-internal mutex to ensure mutual exclusion.
	 */
	StreamRC(connection_type& conn) : m_connection(conn){};

	auto submit_async(submit_arg_type const& message)
	{
		return m_connection.submit_async(message);
	}

	auto submit_blocking(submit_arg_type const& message)
	{
		return m_connection.submit_blocking(message);
	}

	void register_reinit(typename connection_type::interface_types::reinit_type&& reinit)
	{
		if constexpr (Check::template has_method_register_reinit<connection_type>::value) {
			m_connection.register_reinit(std::move(reinit));
		} else {
			static_assert(
			    sizeof(connection_type) == 0, "Connection does not support register_reinit!");
		}
	}

	void register_reinit(typename connection_type::interface_types::reinit_type const& reinit)
	{
		if constexpr (Check::template has_method_register_reinit<connection_type>::value) {
			m_connection.register_reinit(reinit);
		} else {
			static_assert(
			    sizeof(connection_type) == 0, "Connection does not support register_reinit!");
		}
	}

	auto get_uploader_reinit() const
	{
		if constexpr (Check::template has_method_get_uploader_reinit<connection_type>::value) {
			return m_connection.get_uploader_reinit();
		} else {
			static_assert(
			    sizeof(connection_type) == 0, "Connection does not support get_uploader_reinit!");
		}
	}

protected:
	connection_type& m_connection;
};

} // namespace hxcomm
