#include <gtest/gtest.h>

#include "hxcomm/common/cerealization_utmessage.h"

#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <cereal/archives/xml.hpp>

#include "test-to_testing_types.h"

using namespace hxcomm::vx;

template <class T>
class CommonSerializationTests : public ::testing::Test
{};

typedef
    typename to_testing_types<instruction::ToFPGADictionary, instruction::FromFPGADictionary>::type
        SerializableTypes;

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
