#pragma once
#define CEREAL_SERIALIZE_FUNCTION_NAME cerealize

#include <cereal/cereal.hpp>

#define EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(CLASS_NAME)                                          \
	template SYMBOL_VISIBLE void CLASS_NAME ::cerealize(cereal::BinaryOutputArchive&);             \
	template SYMBOL_VISIBLE void CLASS_NAME ::cerealize(cereal::BinaryInputArchive&);              \
	template SYMBOL_VISIBLE void CLASS_NAME ::cerealize(cereal::PortableBinaryOutputArchive&);     \
	template SYMBOL_VISIBLE void CLASS_NAME ::cerealize(cereal::PortableBinaryInputArchive&);      \
	template SYMBOL_VISIBLE void CLASS_NAME ::cerealize(cereal::JSONOutputArchive&);               \
	template SYMBOL_VISIBLE void CLASS_NAME ::cerealize(cereal::JSONInputArchive&);                \
	template SYMBOL_VISIBLE void CLASS_NAME ::cerealize(cereal::XMLOutputArchive&);                \
	template SYMBOL_VISIBLE void CLASS_NAME ::cerealize(cereal::XMLInputArchive&);
