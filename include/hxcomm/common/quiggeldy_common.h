#pragma once
#include "hate/visibility.h"
#include <chrono>
#include <cstddef>
#include <map>
#include <optional>
#include <string>
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
char const* get_munge_socket() SYMBOL_VISIBLE;

/**
 * Check for availability of munge by checking if the default munge socket (or
 * the one specified by the environment does exist). bool is_munge_available).
 *
 * This is useful to disable munge authentication in testing environments where
 * munge is not running.
 *
 * @return whether munge is available or not.
 */
bool is_munge_available() SYMBOL_VISIBLE;

#ifdef USE_MUNGE_AUTH

// Helper function to create a custom munge context that adjusts the socket
// path.
munge_ctx_t munge_ctx_setup() SYMBOL_VISIBLE;

#endif // USE_MUNGE_AUTH

/**
 * Helper function to verify a JSON-Web-Token.
 * @param token JWT to verify.
 * @param public_key Public key to verify signature.
 * @param expiration_grace_time Grace time for which a expired token is marked valid.
 * @param claims Map in which claims are returned.
 * @param encryption_method Encryption method used for the JWT.
 * * @throw Throws logic_error if the token is invalid.
 */
void verify_jwt(
    std::string const& token,
    std::string const& public_key,
    std::chrono::seconds expiration_grace_time,
    std::optional<std::map<std::string, std::string>> claims,
    std::string encryption_method) SYMBOL_VISIBLE;

} // namespace hxcomm
