#include <gtest/gtest.h>

#include "hxcomm/common/cerealization_connection_time_info.h"
#include "hxcomm/common/cerealization_utmessage.h"

#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <cereal/archives/xml.hpp>

#include <fstream>
#include <type_traits>

#include "test-to_testing_types.h"

using namespace hxcomm::vx;

template <class T>
class CommonSerializationTests : public ::testing::Test
{};

typedef typename to_testing_types<
    instruction::ToFPGADictionary,
    instruction::FromFPGADictionary,
    hxcomm::ConnectionTimeInfo>::type SerializableTypes;

TYPED_TEST_CASE(CommonSerializationTests, SerializableTypes);

TYPED_TEST(CommonSerializationTests, IsDefaultConstructible)
{
	TypeParam obj;
	static_cast<void>(&obj);
}

TYPED_TEST(CommonSerializationTests, IsAssignable)
{
	TypeParam obj1, obj2;
	obj1 = obj2;
}

#define SERIALIZATION_TEST(OArchive, IArchive)                                                     \
	TYPED_TEST(CommonSerializationTests, HasSerialization##IArchive##OArchive)                     \
	{                                                                                              \
		TypeParam obj1, obj2;                                                                      \
                                                                                                   \
		if constexpr (std::is_standard_layout_v<TypeParam>) {                                      \
			std::ifstream urandom("/dev/urandom", std::ios_base::in | std::ios_base::binary);      \
			char* raw = reinterpret_cast<char*>(&obj1);                                            \
                                                                                                   \
			/* Fill object with random data so we detect if serialization misses some members */   \
			for (std::size_t i = 0; i < sizeof(TypeParam); ++i) {                                  \
				urandom >> raw[i];                                                                 \
			}                                                                                      \
		}                                                                                          \
                                                                                                   \
		using namespace cereal;                                                                    \
		std::ostringstream ostream;                                                                \
		{                                                                                          \
			OArchive oa(ostream);                                                                  \
			oa(obj1);                                                                              \
		}                                                                                          \
                                                                                                   \
		std::istringstream istream(ostream.str());                                                 \
		{                                                                                          \
			IArchive ia(istream);                                                                  \
			ia(obj2);                                                                              \
		}                                                                                          \
		/* This does only test that Serialization does not insert wrong values                     \
		   but does not check coverage since both instances are default constructed.               \
		   Coverage check is done in each container's test file. */                                \
		ASSERT_EQ(obj2, obj1);                                                                     \
	}

SERIALIZATION_TEST(BinaryOutputArchive, BinaryInputArchive)
SERIALIZATION_TEST(PortableBinaryOutputArchive, PortableBinaryInputArchive)
SERIALIZATION_TEST(JSONOutputArchive, JSONInputArchive)
SERIALIZATION_TEST(XMLOutputArchive, XMLInputArchive)

#undef SERIALIZATION_TEST
