#include "hxcomm/utmessage.h"

namespace hxcomm {
ut_message_t UTMessageFactory::reset_msg()
{
	return static_cast<ut_message_t>(alphabet::reset) << header_shift;
}

ut_message_t UTMessageFactory::set_clockscaler(ut_scaler_t factor)
{
	return static_cast<ut_message_t>(alphabet::scaler) << header_shift | factor;
}

ut_message_t UTMessageFactory::set_ir(ut_ins_t ins)
{
	return static_cast<ut_message_t>(alphabet::ins) << header_shift | ins.to_ullong();
}

ut_message_t UTMessageFactory::set_dr(ut_data_t data, ut_data_len_t len, bool keep_response)
{
	return static_cast<ut_message_t>(alphabet::data) << header_shift |
	       static_cast<ut_message_t>(keep_response) << 48 | len.to_ullong() << 40 |
	       data.to_ullong();
}

ut_message_t UTMessageFactory::reset_timestamp()
{
	return static_cast<ut_message_t>(alphabet::reset_time) << header_shift;
}

ut_message_t UTMessageFactory::wait_until(ut_time_t time)
{
	return static_cast<ut_message_t>(alphabet::wait_until) << header_shift |
	       static_cast<ut_message_t>(time);
}

ut_message_t UTMessageFactory::init_msg(bool val)
{
	return static_cast<ut_message_t>(alphabet::init) << header_shift |
	       static_cast<ut_message_t>(val);
}

} // hxcomm
