#pragma once

#include <memory>
#include <type_traits>

#include "hxcomm/common/logger.h"
#include "hxcomm/common/quiggeldy_connection.h"
#include "hxcomm/common/quiggeldy_interface_types.h"
#include "hxcomm/common/stream_rc.h"

namespace hxcomm {

/**
 * Proxy object that can be used independently from a connection to register a
 * reinit program to be run whenever control of the hardware resources is
 * relinquished for some time.
 *
 * Note: On direct-access connections this is a no-op since those connections
 * do not support the notion of relinquishing control. Right now, only
 * `QuiggeldyConnection` makes use of this.
 */
template <typename QuiggeldyConnection>
class RegisterReinit
{
public:
	using quiggeldy_connection_type = QuiggeldyConnection;
	using uploader_reinit_type = typename quiggeldy_connection_type::uploader_reinit_type;
	using interface_types = typename quiggeldy_connection_type::interface_types;
	using reinit_type = typename interface_types::reinit_type;

	RegisterReinit() = delete;
	template <typename Connection>
	RegisterReinit(Connection const&);
	RegisterReinit(RegisterReinit const&) = default;
	RegisterReinit(RegisterReinit&&) = default;

	/**
	 * Register a reinit program to be used on the remote site.
	 *
	 * Takes ownership of the program.
	 */
	void operator()(reinit_type&&);

	/**
	 * Register a reinit program to be used on the remote site.
	 *
	 * Copies the program.
	 */
	void operator()(reinit_type const&);

private:
	log4cxx::Logger* m_log;
	bool m_connection_supports_reinit;
	std::weak_ptr<uploader_reinit_type> m_uploader_reinit;

	template <typename Connection>
	inline static constexpr bool supports_reinit_v =
	    std::is_same_v<Connection, quiggeldy_connection_type>;

	template <typename Connection, std::enable_if_t<supports_reinit_v<Connection>, int> = 0>
	void setup_uploader(Connection const& connection)
	{
		// preserve constness by declaring stream const even though we have to remote const-ness for
		// construction -> unfortunately there is no custom constructor for const
		StreamRC<quiggeldy_connection_type> const stream(const_cast<Connection&>(connection));
		m_uploader_reinit = stream.get_uploader_reinit();
	}

	template <typename Connection, std::enable_if_t<!supports_reinit_v<Connection>, int> = 0>
	void setup_uploader(Connection const&)
	{}
};

} // namespace hxcomm

#include "hxcomm/common/register_reinit.tcc"
