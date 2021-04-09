#pragma once

#include "SF/Archive.hpp"
#include "SF/string.hpp"
#include "SF/vector.hpp"
#include "cereal/archives/binary.hpp"
#include <sstream>
#include <string>

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
	auto create_buffer = [] {
		constexpr std::size_t bufsize = 32     /* MiB */
		                                * 1024 /* KiB */
		                                * 1024 /* Byte */
		                                * 2 /* double to account for imperfect serialization */;
		std::string buf;
		buf.reserve(bufsize);
		return buf;
	};

	if (ar.isWrite()) {
		std::ostringstream os{create_buffer()};
		// std::ostringstream os;
		{
			cereal::BinaryOutputArchive cereal_archive(os);

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
		auto buf = create_buffer();
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
			cereal::BinaryInputArchive cereal_archive(is);

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
