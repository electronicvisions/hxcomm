#pragma once

#include <sstream>
#include <string>

#include "SF/Archive.hpp"
#include "SF/string.hpp"
#include "SF/vector.hpp"

#include "cereal/archives/portable_binary.hpp"

// time statistics
#include "hxcomm/common/logger.h"
#include <chrono>

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

			[[maybe_unused]] auto start = std::chrono::high_resolution_clock::now();
			cereal_archive(cerealizable);
			[[maybe_unused]] auto stop = std::chrono::high_resolution_clock::now();
			HXCOMM_LOG_DEBUG(
			    log,
			    "cereal (object -> archive): "
			        << std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count()
			        << "ms");
		}

		{
			[[maybe_unused]] auto start = std::chrono::high_resolution_clock::now();
			// clang-format off
			ar & os.str();
			// clang-format on
			[[maybe_unused]] auto stop = std::chrono::high_resolution_clock::now();
			HXCOMM_LOG_DEBUG(
			    log,
			    "SF (object -> archive): "
			        << std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count()
			        << "ms");
		}

	} else if (ar.isRead()) {
		std::string buf;
		{
			[[maybe_unused]] auto start = std::chrono::high_resolution_clock::now();
			// clang-format off
			ar & buf;
			// clang-format on
			[[maybe_unused]] auto stop = std::chrono::high_resolution_clock::now();
			HXCOMM_LOG_DEBUG(
			    log,
			    "SF (archive -> object): "
			        << std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count()
			        << "ms");
		}

		std::istringstream is(buf);
		{
			cereal::PortableBinaryInputArchive cereal_archive(is);

			[[maybe_unused]] auto start = std::chrono::high_resolution_clock::now();
			cereal_archive(cerealizable);
			[[maybe_unused]] auto stop = std::chrono::high_resolution_clock::now();
			HXCOMM_LOG_DEBUG(
			    log,
			    "cereal (archive -> object): "
			        << std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count()
			        << "ms");
		}

	} else {
		throw std::logic_error("SF::Archive is neither writable nor readable!");
	}
}

} // namespace SF
