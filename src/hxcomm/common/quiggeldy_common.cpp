#include "hxcomm/common/quiggeldy_common.h"
#include "jwt-cpp/jwt.h"
#include "picojson/picojson.h"

#include "hxcomm/common/logger.h"
#include <filesystem>

#include <chrono>
#include <iostream>

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

void verify_jwt(
    std::string const& token,
    std::string const& public_key,
    std::chrono::seconds expiration_grace_time,
    std::optional<std::map<std::string, std::string>> claims,
    std::string encryption_method)
{
	auto log = log4cxx::Logger::getLogger("hxcomm.verify_jwt");
	HXCOMM_LOG_TRACE(log, "Verification of JWT.");

	auto verifyer = jwt::verify();

	if (encryption_method == "RSA256") {
		verifyer.allow_algorithm(jwt::algorithm::rs256(public_key, "", "", ""));
	} else if (encryption_method == "RS384") {
		verifyer.allow_algorithm(jwt::algorithm::rs384(public_key, "", "", ""));
	} else if (encryption_method == "RS512") {
		verifyer.allow_algorithm(jwt::algorithm::rs512(public_key, "", "", ""));
	} else if (encryption_method == "ES256") {
		verifyer.allow_algorithm(jwt::algorithm::es256(public_key, "", "", ""));
	} else if (encryption_method == "ES384") {
		verifyer.allow_algorithm(jwt::algorithm::es384(public_key, "", "", ""));
	} else if (encryption_method == "ES512") {
		verifyer.allow_algorithm(jwt::algorithm::es512(public_key, "", "", ""));
	} else if (encryption_method == "ES256k") {
		verifyer.allow_algorithm(jwt::algorithm::es256k(public_key, "", "", ""));
	} else if (encryption_method == "ED25519") {
		verifyer.allow_algorithm(jwt::algorithm::ed25519(public_key, "", "", ""));
	} else if (encryption_method == "PS256") {
		verifyer.allow_algorithm(jwt::algorithm::ps256(public_key, "", "", ""));
	} else if (encryption_method == "PS384") {
		verifyer.allow_algorithm(jwt::algorithm::ps384(public_key, "", "", ""));
	} else if (encryption_method == "PS512") {
		verifyer.allow_algorithm(jwt::algorithm::ps512(public_key, "", "", ""));
	} else {
		throw std::runtime_error("No encryption method for JWT-Token provided.");
	}

	verifyer.expires_at_leeway(static_cast<size_t>(expiration_grace_time.count()));


	auto decoded_token = jwt::decode(token);

	if (claims) {
		std::stringstream token_payload;
		token_payload << decoded_token.get_payload();

		picojson::value json_value;
		std::string json_parse_error = picojson::parse(json_value, token_payload);
		if (!json_parse_error.empty()) {
			throw std::runtime_error("Parsing of JSON file failed.");
		}

		picojson::object json_object = json_value.get<picojson::object>();
		for (auto const& [claim, value] : json_object) {
			(*claims)[claim] = value.to_str();
		}
	}

	try {
		verifyer.verify(decoded_token);
		HXCOMM_LOG_TRACE(log, "Verification successful.");
	} catch (std::system_error const& e) {
		std::stringstream ss;
		ss << "Verification failed due to: " << e.what();
		HXCOMM_LOG_TRACE(log, ss.str());
		throw std::runtime_error(ss.str());
	}
}

} // namespace hxcomm
