#pragma once

#include <bitset>
#include <stdint.h>

namespace hxcomm {

typedef uint8_t ut_scaler_t;
typedef std::bitset<7> ut_ins_t;
typedef std::bitset<33> ut_data_t;
typedef std::bitset<6> ut_data_len_t;
typedef uint64_t ut_message_t;
typedef uint32_t ut_time_t;


/// Collection of static functions to generate UT messages.
class UTMessageFactory
{
private:
	static const size_t header_shift = 56;
	enum struct alphabet : ut_message_t // avoid casting later
	{
		reset = 0,
		scaler = 1,
		ins = 2,
		data = 3,
		reset_time = 4,
		wait_until = 5,
		init = 6
	};

public:
	static ut_message_t reset_msg();
	static ut_message_t set_clockscaler(ut_scaler_t factor);
	static ut_message_t set_ir(ut_ins_t ins);
	static ut_message_t set_dr(ut_data_t data, ut_data_len_t len, bool keep_response);
	static ut_message_t reset_timestamp();
	static ut_message_t wait_until(ut_time_t time);
	static ut_message_t init_msg(bool val);
};

} // namespace hxcomm
