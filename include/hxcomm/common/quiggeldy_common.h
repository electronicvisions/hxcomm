#pragma once

#ifdef USE_MUNGE_AUTH
#include <munge.h>
#endif

namespace hxcomm {

/**
 * Max message length should be at least as long as the FPGA memory filled
 * with results (and then some).
 */
static constexpr size_t quiggeldy_max_message_length = 1280 * 1024 * 1024;

/**
 * Get the munge socket to connect to.
 *
 * @return Path to munge socket to use. Either the default or the one specified
 * via environment.
 */
inline char const* get_munge_socket();

/**
 * Check for availability of munge by checking if the default munge socket (or
 * the one specified by the environment does exist). bool is_munge_available).
 *
 * This is useful to disable munge authentication in testing environments where
 * munge is not running.
 *
 * @return whether munge is available or not.
 */
inline bool is_munge_available();

#ifdef USE_MUNGE_AUTH

// Helper function to create a custom munge context that adjusts the socket
// path.
inline munge_ctx_t munge_ctx_setup();

#endif // USE_MUNGE_AUTH

} // namespace hxcomm

#include "hxcomm/common/quiggeldy_common.tcc"
