#include "hxcomm/common/logger.h"

#include <filesystem>

namespace hxcomm {

char const* get_munge_socket()
{
	// TODO: Move name of env variable #3764
	static char const* munge_opt_socket_env = std::getenv("MUNGE_OPT_SOCKET");
	return munge_opt_socket_env != nullptr ? munge_opt_socket_env : "/run/munge/munge.socket.2";
}

bool is_munge_available()
{
#ifdef USE_MUNGE_AUTH
	auto munge_socket = get_munge_socket();
	return std::filesystem::exists(munge_socket);
#else
    return false;
#endif
}

#ifdef USE_MUNGE_AUTH
munge_ctx_t munge_ctx_setup()
{
	auto log = log4cxx::Logger::getLogger("hxcomm.munge_ctx_setup");

	munge_ctx_t ctx = munge_ctx_create();

	auto munge_opt_socket = get_munge_socket();

	HXCOMM_LOG_TRACE(log, "Communicating via munge socket: " << munge_opt_socket);

	if (munge_ctx_set(ctx, MUNGE_OPT_SOCKET, munge_opt_socket) != EMUNGE_SUCCESS) {
		std::stringstream ss;
		ss << "Could not set munge socket: " << munge_opt_socket;
		HXCOMM_LOG_ERROR(log, ss.str());
		throw std::runtime_error(ss.str());
	}

	return ctx;
}
#endif

} // namespace hxcomm
