#include "hxcomm/vx/zeromockconnection.h"

#include "hate/variant.h"
#include "hxcomm/common/zeromockconnection_impl.tcc"

namespace hxcomm {

namespace detail {

ZeroMockProcessMessage<hxcomm::vx::ConnectionParameter>::ZeroMockProcessMessage(
    receive_queue_type& receive_queue, bool& halt) :
    m_receive_queue(receive_queue), m_halt(halt)
{}

void ZeroMockProcessMessage<hxcomm::vx::ConnectionParameter>::operator()(
    send_message_type const& message)
{
	auto const process_loopback =
	    [this](hxcomm::vx::UTMessageToFPGA<hxcomm::vx::instruction::system::Loopback> const& msg) {
		    if (msg.decode() == hxcomm::vx::instruction::system::Loopback::halt) {
			    m_halt = true;
		    }
		    m_receive_queue.emplace_back(
		        hxcomm::vx::UTMessageFromFPGA<hxcomm::vx::instruction::from_fpga_system::Loopback>(
		            hxcomm::vx::instruction::from_fpga_system::Loopback::Payload(msg.decode())));
	    };
	auto const process_jtag =
	    [this](
	        hxcomm::vx::UTMessageToFPGA<hxcomm::vx::instruction::to_fpga_jtag::Data> const& msg) {
		    auto const keep_response = msg.get_payload().test(
		        hxcomm::vx::instruction::to_fpga_jtag::Data::size -
		        hxcomm::vx::instruction::to_fpga_jtag::Data::padded_num_bits_keep_response);
		    if (!keep_response) { // is no read
			    return;
		    }

		    auto const response =
		        hxcomm::vx::UTMessageFromFPGA<hxcomm::vx::instruction::jtag_from_hicann::Data>(
		            hxcomm::vx::instruction::jtag_from_hicann::Data::Payload(0));
		    m_receive_queue.emplace_back(response);
	    };
	auto const process_omnibus =
	    [this](hxcomm::vx::UTMessageToFPGA<hxcomm::vx::instruction::omnibus_to_fpga::Address> const&
	               msg) {
		    if (!msg.get_payload().test(
		            sizeof(uint32_t) *
		            (CHAR_BIT /* address */ + 1 /* byte enables */))) { // is no read
			    return;
		    }
		    auto const response =
		        hxcomm::vx::UTMessageFromFPGA<hxcomm::vx::instruction::omnibus_from_fpga::Data>(0);
		    m_receive_queue.emplace_back(response);
	    };
	std::visit(
	    hate::overloaded{process_loopback, process_jtag, process_omnibus, [](auto&&) {}}, message);
}

} // namespace detail

template class ZeroMockConnection<hxcomm::vx::ConnectionParameter>;

} // namepace hxcomm
