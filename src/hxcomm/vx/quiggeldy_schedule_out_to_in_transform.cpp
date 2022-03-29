#include "hxcomm/vx/quiggeldy_schedule_out_to_in_transform.h"

#include <variant>

namespace hxcomm::vx::detail {

QuiggeldyScheduleOutToInTransform::request_type QuiggeldyScheduleOutToInTransform::operator()(
    response_type const& response, request_type const& snapshot)
{
	// simple unchecked transform from Omnibus reads to Omnibus writes

	// filter all read requests
	request_type addresses;
	for (auto const& ins : snapshot) {
		if (!std::holds_alternative<UTMessageToFPGA<instruction::omnibus_to_fpga::Address>>(ins)) {
			continue;
		}
		auto address = std::get<UTMessageToFPGA<instruction::omnibus_to_fpga::Address>>(ins);
		auto address_payload = address.decode();
		if (address_payload.get_is_read()) {
			address_payload.set_is_read(false);
			address.encode(address_payload);
			addresses.push_back(address);
		}
	}

	// filter all read responses
	request_type data;
	for (auto const& resp : response) {
		if (!std::holds_alternative<UTMessageFromFPGA<instruction::omnibus_from_fpga::Data>>(
		        resp)) {
			continue;
		}
		data.push_back(UTMessageToFPGA<instruction::omnibus_to_fpga::Data>(
		    std::get<UTMessageFromFPGA<instruction::omnibus_from_fpga::Data>>(resp)
		        .decode()
		        .value()));
	}

	// combine addresses and data to write accesses
	if (addresses.size() != data.size()) {
		throw std::runtime_error("Transform to Omnibus writes unsuccessful, mismatch in number "
		                         "of read requests to responses.");
	}
	request_type ret;
	for (size_t i = 0; i < addresses.size(); ++i) {
		ret.push_back(addresses.at(i));
		ret.push_back(data.at(i));
	}
	// ensure writes are successful
	ret.push_back(
	    UTMessageToFPGA<instruction::timing::Barrier>(instruction::timing::Barrier::omnibus));
	return ret;
}

} // namespace hxcomm::vx::detail
