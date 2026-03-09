#include "hxcomm/common/execute_messages_types.h"
#include "hxcomm/common/logger.h"
#include "hxcomm/common/multiconnection.h"

#include "hxcomm/common/stream.h"
#include <future>
#include <log4cxx/logger.h>

#include "hxcomm/common/visit_connection.h"

namespace hxcomm {

namespace detail {

template <typename Connection>
struct ExecuteMessagesReturnType<MultiConnection<Connection>>
{
	using type = std::vector<execute_messages_return_t<Connection>>;
};

template <typename Connection>
struct ExecuteMessagesArgumentType<MultiConnection<Connection>>
{
	using type = std::vector<execute_messages_argument_t<Connection>>;
};

template <typename Connection>
struct ExecuteMessagesArgumentReferenceWrappedType<MultiConnection<Connection>>
{
	using type = std::vector<execute_messages_argument_reference_wrapped_t<Connection>>;
};

template <typename Connection>
struct ExecutorMessages<hxcomm::MultiConnection<Connection>>
{
	using connection_type = hxcomm::MultiConnection<Connection>;
	using sub_connection_type = Connection;

	using return_type = execute_messages_return_t<connection_type>;
	using messages_type = execute_messages_argument_t<connection_type>;
	using message_type_wrapped = execute_messages_argument_reference_wrapped_t<connection_type>;

	using sub_return_type = execute_messages_return_t<sub_connection_type>;
	using sub_messages_type = execute_messages_argument_t<sub_connection_type>;
	using sub_send_halt_message_type = typename sub_connection_type::send_halt_message_type;

	/**
	 * Vector of messages given by value.
	 */
	return_type operator()(connection_type& multi_connection, messages_type const& messages)
	{
		log4cxx::LoggerPtr log = log4cxx::Logger::getLogger("hxcomm.execute_messages");

		if (messages.size() != multi_connection.size()) {
			throw std::invalid_argument(
			    "Supplied number of message lists doesn't match multi-connection size.");
		}

		std::vector<std::future<sub_return_type>> futures;

		auto execute_message_sub_connection = [&multi_connection, &messages, &log](size_t index) {
			auto& connection = multi_connection[index];

			Stream<sub_connection_type> stream(connection);
			auto const time_begin = connection.get_time_info();


			stream.add(messages.at(index).begin(), messages.at(index).end());
			stream.add(sub_send_halt_message_type());
			stream.commit();

			stream.run_until_halt();

			auto const responses = stream.receive_all();
			auto const time_difference = connection.get_time_info() - time_begin;


			HXCOMM_LOG_INFO(
			    log, "Executed messages(" << messages.size() << ") and got responses("
			                              << responses.size()
			                              << ") with time expenditure: " << std::endl
			                              << time_difference << " on connection " << index << ".");

			return std::make_pair(responses, time_difference);
		};

		for (size_t i = 0; i < multi_connection.size(); i++) {
			futures.push_back(std::async(std::launch::async, execute_message_sub_connection, i));
		}

		return_type result;
		for (auto& future : futures) {
			result.push_back(future.get());
		}

		return result;
	}

	/**
	 * Vector of messages given with reference wrapper.
	 */
	return_type operator()(connection_type& multi_connection, message_type_wrapped const& messages)
	{
		log4cxx::LoggerPtr log = log4cxx::Logger::getLogger("hxcomm.execute_messages");

		if (messages.size() != multi_connection.size()) {
			throw std::invalid_argument(
			    "Supplied number of message lists doesn't match multi-connection size.");
		}

		std::vector<std::future<sub_return_type>> futures;

		auto execute_message_sub_connection = [&multi_connection, &messages, &log](size_t index) {
			auto& connection = multi_connection[index];

			Stream<sub_connection_type> stream(connection);
			auto const time_begin = connection.get_time_info();


			stream.add(messages.at(index).get().begin(), messages.at(index).get().end());
			stream.add(sub_send_halt_message_type());
			stream.commit();

			stream.run_until_halt();

			auto const responses = stream.receive_all();
			auto const time_difference = connection.get_time_info() - time_begin;

			HXCOMM_LOG_INFO(
			    log, "Executed messages(" << messages.size() << ") and got responses("
			                              << responses.size()
			                              << ") with time expenditure: " << std::endl
			                              << time_difference << " on connection " << index << ".");

			return std::make_pair(responses, time_difference);
		};

		for (size_t i = 0; i < multi_connection.size(); i++) {
			futures.push_back(std::async(std::launch::async, execute_message_sub_connection, i));
		}

		return_type result;
		for (auto& future : futures) {
			result.push_back(future.get());
		}

		return result;
	}
};


} // namespace detail

} // namespace hxcomm