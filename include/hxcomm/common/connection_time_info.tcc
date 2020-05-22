#include <type_traits>
#include "hate/timer.h"

namespace hxcomm {

inline std::ostream& operator<<(std::ostream& os, ConnectionTimeInfo const& data)
{
	os << "ConnectionTimeInfo(" << std::endl;
	os << "\tencode_duration:    " << hate::to_string(data.encode_duration) << std::endl;
	os << "\tdecode_duration:    " << hate::to_string(data.decode_duration) << std::endl;
	os << "\tcommit_duration:    " << hate::to_string(data.commit_duration) << std::endl;
	os << "\texecution_duration: " << hate::to_string(data.execution_duration) << std::endl;
	os << ")";
	return os;
}

inline ConnectionTimeInfo& ConnectionTimeInfo::operator-=(ConnectionTimeInfo const& other)
{
	encode_duration -= other.encode_duration;
	decode_duration -= other.decode_duration;
	commit_duration -= other.commit_duration;
	execution_duration -= other.execution_duration;
	return *this;
}

inline ConnectionTimeInfo ConnectionTimeInfo::operator-(ConnectionTimeInfo const& other) const
{
	ConnectionTimeInfo ret(*this);
	ret -= other;
	return ret;
}

inline ConnectionTimeInfo& ConnectionTimeInfo::operator+=(ConnectionTimeInfo const& other)
{
	encode_duration += other.encode_duration;
	decode_duration += other.decode_duration;
	commit_duration += other.commit_duration;
	execution_duration += other.execution_duration;
	return *this;
}

inline ConnectionTimeInfo ConnectionTimeInfo::operator+(ConnectionTimeInfo const& other) const
{
	ConnectionTimeInfo ret(*this);
	ret += other;
	return ret;
}

inline bool ConnectionTimeInfo::operator==(ConnectionTimeInfo const& other) const
{
	return encode_duration == other.encode_duration && decode_duration == other.decode_duration &&
	       commit_duration == other.commit_duration &&
	       execution_duration == other.execution_duration;
}

inline bool ConnectionTimeInfo::operator!=(ConnectionTimeInfo const& other) const
{
	return !(*this == other);
}

} // namespace hxcomm
