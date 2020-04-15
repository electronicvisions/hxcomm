#pragma once

#include <sstream>
#include <string>

#include "SF/Archive.hpp"

#include "cereal/archives/portable_binary.hpp"

namespace SF {

/**
 * Serialize any object that can be serialized by cereal to SF.
 */
template <class T>
void translate_sf_cereal(Archive& ar, T& cerealizable)
{
	if (ar.isWrite()) {
		std::ostringstream os;
		{
			cereal::PortableBinaryOutputArchive cereal_archive(os);

			cereal_archive(cerealizable);
		}

		ar& os.str();

	} else if (ar.isRead()) {
		std::string buf;
		ar& buf;

		std::istringstream is(buf);
		{
			cereal::PortableBinaryInputArchive cereal_archive(is);

			cereal_archive(cerealizable);
		}

	} else {
		throw std::logic_error("SF::Archive is neither writable nor readable!");
	}
}

} // namespace SF
