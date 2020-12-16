#pragma once

#include "hate/type_list.h"
#include "hxcomm/common/logger.h"
#include "hxcomm/common/quiggeldy_interface_types.h"
#include "hxcomm/common/stream_rc.h"
#include <functional>
#include <memory>
#include <optional>
#include <type_traits>
#include <variant>

namespace hxcomm {

namespace detail {

template <typename T>
struct variant_to_ref
{
	static_assert(sizeof(T) == 0, "Should not be constructed.");
};

template <typename... T>
struct variant_to_ref<std::variant<T...>>
{
	using type = std::variant<std::reference_wrapper<T>...>;
};

template <typename T>
using variant_to_ref_t = typename variant_to_ref<T>::type;

} // namespace detail

/**
 * Proxy object that can be used independently from a connection to register a
 * reinit program to be run whenever control of the hardware resources is
 * relinquished for some time.
 *
 * Note: On direct-access connections this is a no-op since those connections
 * do not support the notion of relinquishing control. Right now, only
 * `QuiggeldyConnection` makes use of this.
 */
template <typename QuiggeldyConnection, typename ConnectionVariant>
class ReinitStackEntry
{
public:
	using quiggeldy_connection_type = QuiggeldyConnection;
	using reinit_uploader_type = typename quiggeldy_connection_type::reinit_uploader_type;
	using interface_types = typename quiggeldy_connection_type::interface_types;
	using reinit_type = typename interface_types::reinit_type;
	using reinit_entry_type = typename interface_types::reinit_entry_type;
	using reinit_stack_type = typename quiggeldy_connection_type::reinit_stack_type;

	using connection_ref_variant_type = detail::variant_to_ref_t<ConnectionVariant>;

	ReinitStackEntry() = delete;
	template <typename Connection>
	ReinitStackEntry(Connection&);
	ReinitStackEntry(ConnectionVariant&);
	ReinitStackEntry(ReinitStackEntry const&) = default;
	ReinitStackEntry(ReinitStackEntry&&) = default;
	~ReinitStackEntry();

	/**
	 * Register a reinit program to be used on the remote site.
	 *
	 * Takes ownership of the program.
	 *
	 * @param entry What to upload.
	 * @param enforce Whether to force usage of reinit.
	 */
	void set(reinit_entry_type&& entry, bool enforce = true);

	/**
	 * Register a reinit program to be used on the remote site.
	 *
	 * Copies the program.
	 *
	 * @param entry What to upload.
	 * @param enforce Whether to force usage of reinit.
	 */
	void set(reinit_entry_type const& entry, bool enforce = true);

	/**
	 * Enforce reinit program to be used on the remote site.
	 */
	void enforce();

	/**
	 * Pop this entry from the stack.
	 */
	void pop();

private:
	log4cxx::Logger* m_logger;
	bool m_connection_supports_reinit;
	std::weak_ptr<reinit_uploader_type> m_reinit_uploader;
	std::weak_ptr<reinit_stack_type> m_reinit_stack;
	// position at which this entry resides in the stack, -1 if not part of the stack an
	std::optional<std::size_t> m_idx_in_stack;
	// reference to connection needed for non-quiggeldy connections
	connection_ref_variant_type m_connection_ref;

	template <typename Connection>
	inline static constexpr bool supports_reinit_v =
	    std::is_same_v<std::remove_cvref_t<Connection>, quiggeldy_connection_type>;

	template <typename Connection>
	void setup(Connection& connection);

	void handle_unsupported_connection(reinit_entry_type const&, bool);
};

} // namespace hxcomm

#include "hxcomm/common/reinit_stack_entry.tcc"
