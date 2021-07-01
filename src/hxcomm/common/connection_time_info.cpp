#include "hxcomm/common/connection_time_info.h"

#include "hate/timer.h"
#include "hxcomm/cerealization.h"
#include <type_traits>
#include <cereal/types/chrono.hpp>

namespace hxcomm {

std::ostream& operator<<(std::ostream& os, ConnectionTimeInfo const& data)
{
	os << "ConnectionTimeInfo(" << std::endl;
	os << "\tencode_duration:    " << hate::to_string(data.encode_duration) << std::endl;
	os << "\tdecode_duration:    " << hate::to_string(data.decode_duration) << std::endl;
	os << "\tcommit_duration:    " << hate::to_string(data.commit_duration) << std::endl;
	os << "\texecution_duration: " << hate::to_string(data.execution_duration) << std::endl;
	os << ")";
	return os;
}

ConnectionTimeInfo& ConnectionTimeInfo::operator-=(ConnectionTimeInfo const& other)
{
	encode_duration -= other.encode_duration;
	decode_duration -= other.decode_duration;
	commit_duration -= other.commit_duration;
	execution_duration -= other.execution_duration;
	return *this;
}

ConnectionTimeInfo ConnectionTimeInfo::operator-(ConnectionTimeInfo const& other) const
{
	ConnectionTimeInfo ret(*this);
	ret -= other;
	return ret;
}

ConnectionTimeInfo& ConnectionTimeInfo::operator+=(ConnectionTimeInfo const& other)
{
	encode_duration += other.encode_duration;
	decode_duration += other.decode_duration;
	commit_duration += other.commit_duration;
	execution_duration += other.execution_duration;
	return *this;
}

ConnectionTimeInfo ConnectionTimeInfo::operator+(ConnectionTimeInfo const& other) const
{
	ConnectionTimeInfo ret(*this);
	ret += other;
	return ret;
}

bool ConnectionTimeInfo::operator==(ConnectionTimeInfo const& other) const
{
	return encode_duration == other.encode_duration && decode_duration == other.decode_duration &&
	       commit_duration == other.commit_duration &&
	       execution_duration == other.execution_duration;
}

bool ConnectionTimeInfo::operator!=(ConnectionTimeInfo const& other) const
{
	return !(*this == other);
}

template <typename Archive>
void ConnectionTimeInfo::serialize(Archive& ar, std::uint32_t const)
{
	ar(encode_duration, decode_duration, commit_duration, execution_duration);
}

} // namespace hxcomm

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(hxcomm::ConnectionTimeInfo)
CEREAL_CLASS_VERSION(hxcomm::ConnectionTimeInfo, 0)
