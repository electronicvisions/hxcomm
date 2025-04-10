#include "gtest/gtest.h"

#include "hxcomm/common/logger.h"
#include "hxcomm/common/quiggeldy_utility.h"
#include "hxcomm/common/stream_rc.h"
#include "hxcomm/vx/quiggeldy_connection.h"
#include "hxcomm/vx/reinit_stack_entry.h"

#include "jwt-cpp/jwt.h"

#include <cerrno>
#include <charconv>
#include <chrono>
#include <cstring>
#include <future>
#include <optional>
#include <thread>
#include <utility>

#include <openssl/evp.h>
#include <openssl/pem.h>

#include <sys/types.h>
#include <sys/wait.h>

std::pair<std::string, std::string> generate_rsa_keys(size_t number_bits = 2048)
{
	EVP_PKEY_CTX* ctx = nullptr;
	EVP_PKEY* pkey = nullptr;
	BIO* priv_bio = nullptr;
	BIO* pub_bio = nullptr;
	std::string private_key, public_key;

	do {
		ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
		if (!ctx || EVP_PKEY_keygen_init(ctx) <= 0)
			break;
		if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, static_cast<int>(number_bits)) <= 0)
			break;
		if (EVP_PKEY_keygen(ctx, &pkey) <= 0)
			break;

		// Write private key
		priv_bio = BIO_new(BIO_s_mem());
		if (!priv_bio ||
		    !PEM_write_bio_PrivateKey(priv_bio, pkey, nullptr, nullptr, 0, nullptr, nullptr))
			break;

		// Write public key
		pub_bio = BIO_new(BIO_s_mem());
		if (!pub_bio || !PEM_write_bio_PUBKEY(pub_bio, pkey)) // Writes SubjectPublicKeyInfo
			break;

		// Extract strings
		{
			char* data = nullptr;
			long len = BIO_get_mem_data(priv_bio, &data);
			private_key.assign(data, len);
		}
		{
			char* data = nullptr;
			long len = BIO_get_mem_data(pub_bio, &data);
			public_key.assign(data, len);
		}

	} while (false);

	// Cleanup
	if (ctx)
		EVP_PKEY_CTX_free(ctx);
	if (pkey)
		EVP_PKEY_free(pkey);
	if (priv_bio)
		BIO_free(priv_bio);
	if (pub_bio)
		BIO_free(pub_bio);

	if (private_key.empty() || public_key.empty()) {
		throw std::logic_error("RSA-Keygeneration failed.");
	}
	return std::make_pair(private_key, public_key);
}


TEST(Quiggeldy, SimpleMockModeSynchronous)
{
	using namespace hxcomm;

	auto log = log4cxx::Logger::getLogger("TestQuiggeldy");
	HXCOMM_LOG_TRACE(log, "Starting");
	int status;

	hxcomm::port_t port = get_unused_port();

	int quiggeldy_pid = setup_quiggeldy(
	    "quiggeldy", port, "--mock-mode", "--timeout", "10",
	    hxcomm::is_munge_available() ? "" : "--no-munge");
	using namespace std::literals::chrono_literals;
	std::this_thread::sleep_for(1s);

	auto client = hxcomm::vx::QuiggeldyConnection("127.0.0.1", port);
	StreamRC<decltype(client)> stream{client};

	// synchronous call

	auto response = stream.submit_blocking(decltype(client)::interface_types::request_type());
	ASSERT_EQ(response.first.size(), 0);

	HXCOMM_LOG_TRACE(log, "Killing quiggeldy.");
	kill(quiggeldy_pid, SIGTERM);
	HXCOMM_LOG_TRACE(log, "Waiting for quiggeldy to terminate.");
	waitpid(quiggeldy_pid, &status, 0); // wait for the child to exit
	ASSERT_TRUE(WIFEXITED(status));
	ASSERT_EQ(WEXITSTATUS(status), 0);
}

TEST(Quiggeldy, SimpleMockModeAsynchronous)
{
	using namespace hxcomm;

	auto log = log4cxx::Logger::getLogger("TestQuiggeldy");
	HXCOMM_LOG_TRACE(log, "Starting");

	hxcomm::port_t port = get_unused_port();

	int quiggeldy_pid = setup_quiggeldy(
	    "quiggeldy", port, "--mock-mode", "--timeout", "10",
	    hxcomm::is_munge_available() ? "" : "--no-munge");

	using namespace std::literals::chrono_literals;
	std::this_thread::sleep_for(1s);

	auto client = hxcomm::vx::QuiggeldyConnection("127.0.0.1", port);
	StreamRC<decltype(client)> stream_rc{client};

	// asynchronous calls
	std::vector<typename decltype(client)::future_type> futures;
	for (std::size_t i = 0; i < 100; ++i) {
		futures.push_back(
		    stream_rc.submit_async(decltype(client)::interface_types::request_type()));
	}

	std::size_t idx = 0;
	for (auto& future : futures) {
		HXCOMM_LOG_TRACE(log, "Asserting future #" << idx);
		ASSERT_EQ((*future).first.size(), 0);
		++idx;
	}

	HXCOMM_LOG_TRACE(log, "Killing quiggeldy.");
	kill(quiggeldy_pid, SIGTERM);
	HXCOMM_LOG_TRACE(log, "Waiting for quiggeldy to terminate.");
	int status;
	waitpid(quiggeldy_pid, &status, 0); // wait for the child to exit
	// TODO (bug #3973): known issue, quiggeldy sometimes exists abnormally
	ASSERT_TRUE(WIFEXITED(status) || WIFSIGNALED(status));
	if (WIFEXITED(status)) {
		ASSERT_EQ(WEXITSTATUS(status), 0);
	}
}

TEST(Quiggeldy, SimpleMockModeReinit)
{
	using namespace hxcomm;

	auto log = log4cxx::Logger::getLogger("TestQuiggeldy");
	HXCOMM_LOG_TRACE(log, "Starting");

	hxcomm::port_t port = get_unused_port();

	int quiggeldy_pid = setup_quiggeldy(
	    "quiggeldy", port, "--mock-mode", "--timeout", "10",
	    hxcomm::is_munge_available() ? "" : "--no-munge");

	using namespace std::literals::chrono_literals;
	std::this_thread::sleep_for(1s);

	auto client1 = hxcomm::vx::QuiggeldyConnection("127.0.0.1", port);
	auto client2 = hxcomm::vx::QuiggeldyConnection("127.0.0.1", port);
	StreamRC<decltype(client1)> stream1_rc{client1};
	StreamRC<decltype(client2)> stream2_rc{client2};

	hxcomm::vx::ReinitStackEntry reinit{client1};
	reinit.set(typename decltype(client1)::interface_types::reinit_entry_type{});
	/* no reinit for stream 2 because no reinit program should also work */

	// asynchronous calls
	std::vector<typename decltype(client1)::future_type> futures;
	for (std::size_t i = 0; i < 100; ++i) {
		futures.push_back(
		    stream1_rc.submit_async(decltype(client1)::interface_types::request_type()));
		futures.push_back(
		    stream2_rc.submit_async(decltype(client2)::interface_types::request_type()));
	}

	std::size_t idx = 0;
	for (auto& future : futures) {
		HXCOMM_LOG_TRACE(log, "Asserting future #" << idx);
		ASSERT_EQ((*future).first.size(), 0);
		++idx;
	}

	HXCOMM_LOG_TRACE(log, "Killing quiggeldy.");
	int ret = kill(quiggeldy_pid, SIGTERM);
	ASSERT_EQ(ret, 0) << std::strerror(errno);

	HXCOMM_LOG_TRACE(log, "Waiting for quiggeldy to terminate.");
	int status;
	ret = waitpid(quiggeldy_pid, &status, 0); // wait for the child to exit
	ASSERT_GT(ret, 0) << std::strerror(errno);
	EXPECT_TRUE(
	    WIFEXITED(status) || WIFSIGNALED(status)); // allow for normal and signalled termination
	if (!WIFEXITED(status)) {
		if (WIFSIGNALED(status)) {
			HXCOMM_LOG_WARN(
			    log, "quiggeldy didn't terminate normally, termsig was = " << WTERMSIG(status));
		} else {
			HXCOMM_LOG_ERROR(log, "quiggeldy didn't terminate, status is = " << status);
		}
	} else {
		ASSERT_EQ(WEXITSTATUS(status), 0);
	}
}

TEST(Quiggeldy, ServerRestart)
{
	using namespace hxcomm;

	auto log = log4cxx::Logger::getLogger("TestQuiggeldy");
	HXCOMM_LOG_TRACE(log, "Starting");
	int status;

	hxcomm::port_t port = get_unused_port();

	size_t const num_runs = 20;

	int quiggeldy_pid = setup_quiggeldy(
	    "quiggeldy", port, "--mock-mode", "--timeout", "20",
	    hxcomm::is_munge_available() ? "" : "--no-munge");
	using namespace std::literals::chrono_literals;
	std::this_thread::sleep_for(1s);

	auto const run_client = [port, &log]() -> int {
		auto client = hxcomm::vx::QuiggeldyConnection("127.0.0.1", port);
		StreamRC<decltype(client)> stream{client};

		for (size_t i = 0; i < num_runs; ++i) {
			// calling some remote method
			auto const version = client.get_version_string();
			(void) log;
			HXCOMM_LOG_TRACE(log, "Executed program.");
			std::this_thread::sleep_for(1s);
		}
		return 0;
	};
	auto ret = std::async(std::launch::async, run_client);

	std::this_thread::sleep_for(10s);
	HXCOMM_LOG_TRACE(log, "Killing quiggeldy.");
	kill(quiggeldy_pid, SIGTERM);
	std::this_thread::sleep_for(10s);

	HXCOMM_LOG_TRACE(log, "Starting again");
	quiggeldy_pid = setup_quiggeldy(
	    "quiggeldy", port, "--mock-mode", "--timeout", "20",
	    hxcomm::is_munge_available() ? "" : "--no-munge");
	std::this_thread::sleep_for(10s);

	HXCOMM_LOG_TRACE(log, "Waiting for quiggeldy to terminate.");
	waitpid(quiggeldy_pid, &status, 0); // wait for the child to exit
	ASSERT_TRUE(WIFEXITED(status));
	ASSERT_EQ(WEXITSTATUS(status), 0);

	EXPECT_EQ(ret.get(), 0);
}

TEST(Quiggeldy, UserToken_MissingEncryptionMethod)
{
	using namespace hxcomm;
	auto logger = log4cxx::Logger::getLogger("TestQuiggeldy");
	hxcomm::port_t port = get_unused_port();

	auto [rsa_private_key, rsa_public_key] = generate_rsa_keys();

	HXCOMM_LOG_TRACE(logger, "Starting Quiggeldy-Server.");
	setup_quiggeldy(
	    "quiggeldy", port, "--mock-mode", "--no-munge", "--timeout", "20", "--public-key",
	    rsa_public_key.c_str());

	setenv("QUIGGELDY_IP", "127.0.0.1", 1);
	setenv("QUIGGELDY_PORT", (std::to_string(port)).c_str(), 1);

	HXCOMM_LOG_TRACE(
	    logger, "Establishing connection with user token but without specified encryption method.");

	auto token = jwt::create().set_type("JWT").sign(
	    jwt::algorithm::rs256(rsa_public_key, rsa_private_key, "", ""));
	setenv("QUIGGELDY_TOKEN", token.c_str(), 1);

	EXPECT_THROW(hxcomm::vx::QuiggeldyConnection(), RCF::RemoteException);
}

TEST(Quiggeldy, UserToken_MissingExpiration)
{
	using namespace hxcomm;
	auto logger = log4cxx::Logger::getLogger("TestQuiggeldy");
	hxcomm::port_t port = get_unused_port();

	auto [rsa_private_key, rsa_public_key] = generate_rsa_keys();

	HXCOMM_LOG_TRACE(logger, "Starting Quiggeldy-Server.");
	setup_quiggeldy(
	    "quiggeldy", port, "--mock-mode", "--no-munge", "--timeout", "20", "--public-key",
	    rsa_public_key.c_str(), "--token-encryption=RSA256");

	setenv("QUIGGELDY_IP", "127.0.0.1", 1);
	setenv("QUIGGELDY_PORT", (std::to_string(port)).c_str(), 1);

	// Minimal user token missing expiraton time.
	HXCOMM_LOG_TRACE(logger, "Establishing connection without specified expiration date.");
	auto token = jwt::create().set_type("JWT").sign(
	    jwt::algorithm::rs256(rsa_public_key, rsa_private_key, "", ""));
	setenv("QUIGGELDY_TOKEN", token.c_str(), 1);

	EXPECT_THROW(hxcomm::vx::QuiggeldyConnection(), RCF::RemoteException);
}

TEST(Quiggeldy, UserToken_ExpirationValid)
{
	using namespace hxcomm;
	auto logger = log4cxx::Logger::getLogger("TestQuiggeldy");
	hxcomm::port_t port = get_unused_port();

	auto [rsa_private_key, rsa_public_key] = generate_rsa_keys();

	HXCOMM_LOG_TRACE(logger, "Starting Quiggeldy-Server.");
	setup_quiggeldy(
	    "quiggeldy", port, "--mock-mode", "--no-munge", "--timeout", "20", "--public-key",
	    rsa_public_key.c_str(), "--token-encryption=RSA256");

	setenv("QUIGGELDY_IP", "127.0.0.1", 1);
	setenv("QUIGGELDY_PORT", (std::to_string(port)).c_str(), 1);

	// User token with valid expiration time.
	time_t now = time(0);
	time_t exp = now + 10 * 60; // valid for 10 min.
	HXCOMM_LOG_TRACE(logger, "Establishing connection with valid expiration date.");
	auto token = jwt::create()
	                 .set_type("JWT")
	                 .set_payload_claim("exp", picojson::value(exp))
	                 .sign(jwt::algorithm::rs256(rsa_public_key, rsa_private_key, "", ""));
	setenv("QUIGGELDY_TOKEN", token.c_str(), 1);

	auto client = hxcomm::vx::QuiggeldyConnection();
	StreamRC<decltype(client)> stream{client};

	EXPECT_NO_THROW(stream.submit_blocking(decltype(client)::interface_types::request_type()));
}

TEST(Quiggeldy, UserToken_MultipleClaims)
{
	using namespace hxcomm;
	auto logger = log4cxx::Logger::getLogger("TestQuiggeldy");
	hxcomm::port_t port = get_unused_port();

	auto [rsa_private_key, rsa_public_key] = generate_rsa_keys();

	HXCOMM_LOG_TRACE(logger, "Starting Quiggeldy-Server.");
	setup_quiggeldy(
	    "quiggeldy", port, "--mock-mode", "--no-munge", "--timeout", "20", "--public-key",
	    rsa_public_key.c_str(), "--token-encryption=RSA256");

	setenv("QUIGGELDY_IP", "127.0.0.1", 1);
	setenv("QUIGGELDY_PORT", (std::to_string(port)).c_str(), 1);

	// User token with multiple claims.
	time_t now = time(0);
	time_t iat = now;
	time_t exp = now + 10 * 60;
	HXCOMM_LOG_TRACE(logger, "Establishing connection with multiple claims.");
	auto token = jwt::create()
	                 .set_type("JWT")
	                 .set_issuer("TestIssuer")
	                 .set_payload_claim("sub", picojson::value("Quiggeldy"))
	                 .set_payload_claim("iat", picojson::value(iat))
	                 .set_payload_claim("exp", picojson::value(exp))
	                 .set_payload_claim("name", picojson::value("QuiggeldyTestUser"))
	                 .sign(jwt::algorithm::rs256("", rsa_private_key, "", ""));
	setenv("QUIGGELDY_TOKEN", token.c_str(), 1);

	auto client = hxcomm::vx::QuiggeldyConnection();
	StreamRC<decltype(client)> stream{client};

	EXPECT_NO_THROW(stream.submit_blocking(decltype(client)::interface_types::request_type()));
}

TEST(Quiggeldy, UserToken_Expired)
{
	using namespace hxcomm;
	auto logger = log4cxx::Logger::getLogger("TestQuiggeldy");
	hxcomm::port_t port = get_unused_port();

	auto [rsa_private_key, rsa_public_key] = generate_rsa_keys();

	HXCOMM_LOG_TRACE(logger, "Starting Quiggeldy-Server.");
	setup_quiggeldy(
	    "quiggeldy", port, "--mock-mode", "--no-munge", "--timeout", "20", "--public-key",
	    rsa_public_key.c_str(), "--token-encryption=RSA256");

	setenv("QUIGGELDY_IP", "127.0.0.1", 1);
	setenv("QUIGGELDY_PORT", (std::to_string(port)).c_str(), 1);

	// Expried token.
	time_t now = time(0);
	time_t exp = now - 10 * 60; // invalid for 10 min.
	HXCOMM_LOG_TRACE(
	    logger, "Trying to establish connection and submit request with expired token.");
	auto token = jwt::create()
	                 .set_type("JWT")
	                 .set_payload_claim("exp", picojson::value(exp))
	                 .sign(jwt::algorithm::rs256(rsa_public_key, rsa_private_key, "", ""));
	setenv("QUIGGELDY_TOKEN", token.c_str(), 1);

	EXPECT_THROW(hxcomm::vx::QuiggeldyConnection(), RCF::RemoteException);
}

TEST(Quiggeldy, UserToken_ExpirationGracePeriod)
{
	using namespace hxcomm;
	auto logger = log4cxx::Logger::getLogger("TestQuiggeldy");
	hxcomm::port_t port = get_unused_port();

	auto [rsa_private_key, rsa_public_key] = generate_rsa_keys();

	// Start Quiggeldy-Server with specified grace period.
	time_t grace_period = 20 * 60; // Expire tokens still valid for 20 min.
	setup_quiggeldy(
	    "quiggeldy", port, "--mock-mode", "--no-munge", "--timeout", "20", "--public-key",
	    rsa_public_key.c_str(), "--token-expiration-grace-time",
	    std::to_string(grace_period).c_str(), "--token-encryption=RSA256");

	setenv("QUIGGELDY_IP", "127.0.0.1", 1);
	setenv("QUIGGELDY_PORT", (std::to_string(port)).c_str(), 1);

	// Expried token within grace period.
	time_t now = time(0);
	time_t exp = now + 10 * 60; // valid for 10 min.
	HXCOMM_LOG_TRACE(
	    logger, "Trying to establish connection and submit request with expired token within grace "
	            "period.");
	auto token = jwt::create()
	                 .set_type("JWT")
	                 .set_payload_claim("exp", picojson::value(exp))
	                 .sign(jwt::algorithm::rs256(rsa_public_key, rsa_private_key, "", ""));
	setenv("QUIGGELDY_TOKEN", token.c_str(), 1);

	auto client = hxcomm::vx::QuiggeldyConnection();
	StreamRC<decltype(client)> stream_a{client};

	EXPECT_NO_THROW(stream_a.submit_blocking(decltype(client)::interface_types::request_type()));


	// Expried token not within grace period.
	exp = now - 30 * 60; // invalid for 30 min.
	HXCOMM_LOG_TRACE(
	    logger, "Trying to establish connection and submit request with expired token not within "
	            "grace period.");
	token = jwt::create()
	            .set_type("JWT")
	            .set_payload_claim("exp", picojson::value(exp))
	            .sign(jwt::algorithm::rs256(rsa_public_key, rsa_private_key, "", ""));
	setenv("QUIGGELDY_TOKEN", token.c_str(), 1);

	EXPECT_THROW(hxcomm::vx::QuiggeldyConnection(), RCF::RemoteException);
}