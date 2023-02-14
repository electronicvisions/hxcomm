#pragma once

#include <sstream>
#include <string>

#include "SF/Archive.hpp"
#include "SF/string.hpp"
#include "SF/vector.hpp"

#include "cereal/archives/portable_binary.hpp"

// time statistics
#include "hate/timer.h"
#include "hxcomm/common/logger.h"

namespace SF {

/**
 * Serialize any object that can be serialized by cereal to SF.
 */
template <class T>
void translate_sf_cereal(Archive& ar, T& cerealizable)
{
	auto log = log4cxx::Logger::getLogger(__func__);

	if (ar.isWrite()) {
		std::ostringstream os;
		{
			cereal::PortableBinaryOutputArchive cereal_archive(os);

			[[maybe_unused]] hate::Timer timer;
			cereal_archive(cerealizable);
			HXCOMM_LOG_DEBUG(log, "cereal (object -> archive): " << timer.print());
		}

		{
			[[maybe_unused]] hate::Timer timer;
			// clang-format off
			ar & os.str();
			// clang-format on
			HXCOMM_LOG_DEBUG(log, "SF (object -> archive): " << timer.print());
		}

	} else if (ar.isRead()) {
		std::string buf;
		{
			[[maybe_unused]] hate::Timer timer;
			// clang-format off
			ar & buf;
			// clang-format on
			HXCOMM_LOG_DEBUG(log, "SF (archive -> object): " << timer.print());
		}

		std::istringstream is(buf);
		{
			cereal::PortableBinaryInputArchive cereal_archive(is);

			[[maybe_unused]] hate::Timer timer;
			cereal_archive(cerealizable);
			HXCOMM_LOG_DEBUG(log, "cereal (archive -> object): " << timer.print());
		}

	} else {
		throw std::logic_error("SF::Archive is neither writable nor readable!");
	}
}

} // namespace SF
