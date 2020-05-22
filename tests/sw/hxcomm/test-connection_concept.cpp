#include <gtest/gtest.h>

#include "hate/type_traits.h"

/**
 * Drop-in replacement for `static_assert` performing checks at runtime.
 */
struct exceptionalized_static_assert
{
	exceptionalized_static_assert(bool const cond, char const* what)
	{
		if (cond) {
			static_cast<void>(what);
		} else {
			throw std::logic_error(what);
		}
	}

	exceptionalized_static_assert(bool const cond) :
	    exceptionalized_static_assert(cond, "static_assert failed.")
	{}
};

#define TOKENPASTE(x, y) x##y
#define TOKENPASTE2(x, y) TOKENPASTE(x, y)
/**
 * Replace `static_assert` by runtime checks in order to be able to unittest failures.
 */
#define static_assert(cond, ...)                                                                   \
	exceptionalized_static_assert TOKENPASTE2(_test_, __COUNTER__)                                 \
	{                                                                                              \
		cond, __VA_ARGS__                                                                          \
	}

#include "hxcomm/common/connection.h"


struct Connection
{
	std::initializer_list<hxcomm::Target> supported_targets;
	typedef void send_message_type;
	typedef void receive_message_type;
	typedef void send_halt_message_type;

	Connection(Connection&&);
	Connection(Connection const&) = delete;
	hxcomm::ConnectionTimeInfo get_time_info() const;
};

struct MissingSendMessageTypeConnection
{
	std::initializer_list<hxcomm::Target> supported_targets;
	typedef void receive_message_type;
	typedef void send_halt_message_type;

	MissingSendMessageTypeConnection(MissingSendMessageTypeConnection&&);
	MissingSendMessageTypeConnection(MissingSendMessageTypeConnection const&) = delete;
	hxcomm::ConnectionTimeInfo get_time_info() const;
};

struct MissingReceiveMessageTypeConnection
{
	std::initializer_list<hxcomm::Target> supported_targets;
	typedef void send_message_type;
	typedef void send_halt_message_type;

	MissingReceiveMessageTypeConnection(MissingReceiveMessageTypeConnection&&);
	MissingReceiveMessageTypeConnection(MissingReceiveMessageTypeConnection const&) = delete;
	hxcomm::ConnectionTimeInfo get_time_info() const;
};

struct MissingSendHaltMessageTypeConnection
{
	std::initializer_list<hxcomm::Target> supported_targets;
	typedef void send_message_type;
	typedef void receive_message_type;

	MissingSendHaltMessageTypeConnection(MissingSendHaltMessageTypeConnection&&);
	MissingSendHaltMessageTypeConnection(MissingSendHaltMessageTypeConnection const&) = delete;
	hxcomm::ConnectionTimeInfo get_time_info() const;
};

struct NotMoveableConnection
{
	std::initializer_list<hxcomm::Target> supported_targets;
	typedef void send_message_type;
	typedef void receive_message_type;
	typedef void send_halt_message_type;

	NotMoveableConnection(NotMoveableConnection&&) = delete;
	NotMoveableConnection(NotMoveableConnection const&) = delete;
	hxcomm::ConnectionTimeInfo get_time_info() const;
};

struct CopyableConnection
{
	std::initializer_list<hxcomm::Target> supported_targets;
	typedef void send_message_type;
	typedef void receive_message_type;
	typedef void send_halt_message_type;

	CopyableConnection(CopyableConnection&&);
	CopyableConnection(CopyableConnection const&);
	hxcomm::ConnectionTimeInfo get_time_info() const;
};

struct NoSupportedTargetsConnection
{
	typedef void send_message_type;
	typedef void receive_message_type;
	typedef void send_halt_message_type;

	NoSupportedTargetsConnection(NoSupportedTargetsConnection&&);
	NoSupportedTargetsConnection(NoSupportedTargetsConnection const&) = delete;
	hxcomm::ConnectionTimeInfo get_time_info() const;
};

struct NoGetTimeInfoConnection
{
	std::initializer_list<hxcomm::Target> supported_targets;
	typedef void send_message_type;
	typedef void receive_message_type;
	typedef void send_halt_message_type;

	NoGetTimeInfoConnection(NoGetTimeInfoConnection&&);
	NoGetTimeInfoConnection(NoGetTimeInfoConnection const&) = delete;
};

TEST(ConnectionConcept, General)
{
	EXPECT_NO_THROW(hxcomm::ConnectionConcept<Connection>{});

	EXPECT_THROW(hxcomm::ConnectionConcept<MissingSendMessageTypeConnection>{}, std::logic_error);
	EXPECT_THROW(
	    hxcomm::ConnectionConcept<MissingReceiveMessageTypeConnection>{}, std::logic_error);
	EXPECT_THROW(
	    hxcomm::ConnectionConcept<MissingSendHaltMessageTypeConnection>{}, std::logic_error);

	EXPECT_THROW(hxcomm::ConnectionConcept<NotMoveableConnection>{}, std::logic_error);
	EXPECT_THROW(hxcomm::ConnectionConcept<CopyableConnection>{}, std::logic_error);
	EXPECT_THROW(hxcomm::ConnectionConcept<NoSupportedTargetsConnection>{}, std::logic_error);
	EXPECT_THROW(hxcomm::ConnectionConcept<NoGetTimeInfoConnection>{}, std::logic_error);
}
