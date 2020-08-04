#include "hxcomm/vx/instruction/instruction.h"
#include "hxcomm/vx/payload_random.h"
#include <gtest/gtest.h>

using namespace hxcomm::random;
using namespace hxcomm::vx;

template <class T>
class CommonPayloadTests : public ::testing::Test
{};

template <typename ToTL, typename FromTL>
struct to_testing_types;

template <typename... ToIs, typename... FromIs>
struct to_testing_types<hate::type_list<ToIs...>, hate::type_list<FromIs...>>
{
	typedef ::testing::Types<typename ToIs::Payload..., typename FromIs::Payload...> type;
};

typedef
    typename to_testing_types<instruction::ToFPGADictionary, instruction::FromFPGADictionary>::type
        PayloadTypes;

TYPED_TEST_CASE(CommonPayloadTests, PayloadTypes);

TYPED_TEST(CommonPayloadTests, General)
{
	typedef TypeParam payload_t;

	auto payload = random_payload<payload_t>();

	auto bitstream = payload.encode();
	decltype(payload) decoded;
	decoded.decode(bitstream);
	ASSERT_EQ(payload, decoded);
}
