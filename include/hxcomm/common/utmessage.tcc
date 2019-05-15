#include <iomanip>
#include <boost/variant.hpp>

namespace hxcomm {

template <size_t HeaderAlignment, typename SubwordType, typename PhywordType, typename... Is>
struct ToUTMessageVariant<HeaderAlignment, SubwordType, PhywordType, hate::type_list<Is...> >
{
	typedef boost::variant<
	    UTMessage<HeaderAlignment, SubwordType, PhywordType, hate::type_list<Is...>, Is>...>
	    type;
};


template <size_t HeaderAlignment, typename SubwordType, typename PhywordType, typename... Is>
struct LargestUTMessageSize<HeaderAlignment, SubwordType, PhywordType, hate::type_list<Is...> >
{
	static constexpr size_t value =
	    std::max({UTMessage<HeaderAlignment, SubwordType, PhywordType, hate::type_list<Is...>, Is>::
	                  word_width...});
};


template <size_t HeaderAlignment, typename Dictionary>
struct UTMessageHeaderWidth
{
	static constexpr size_t value = hate::math::round_up_to_multiple(
	    1 /* comma width */ +
	        hate::math::num_bits(
	            hate::type_list_size<Dictionary>::value - 1 /* max. header index */),
	    HeaderAlignment);
};


template <
    size_t HeaderAlignment,
    typename SubwordType,
    typename PhywordType,
    typename Dictionary,
    typename Instruction>
constexpr UTMessage<HeaderAlignment, SubwordType, PhywordType, Dictionary, Instruction>::
    UTMessage() :
    m_data()
{}

template <
    size_t HeaderAlignment,
    typename SubwordType,
    typename PhywordType,
    typename Dictionary,
    typename Instruction>
constexpr UTMessage<HeaderAlignment, SubwordType, PhywordType, Dictionary, Instruction>::UTMessage(
    payload_type const& payload) :
    m_data(payload)
{}

template <
    size_t HeaderAlignment,
    typename SubwordType,
    typename PhywordType,
    typename Dictionary,
    typename Instruction>
constexpr UTMessage<HeaderAlignment, SubwordType, PhywordType, Dictionary, Instruction>::UTMessage(
    typename Instruction::Payload const& payload) :
    m_data(payload.template encode<SubwordType>())
{}

template <
    size_t HeaderAlignment,
    typename SubwordType,
    typename PhywordType,
    typename Dictionary,
    typename Instruction>
constexpr typename UTMessage<HeaderAlignment, SubwordType, PhywordType, Dictionary, Instruction>::
    word_type
    UTMessage<HeaderAlignment, SubwordType, PhywordType, Dictionary, Instruction>::get_raw() const
{
	constexpr word_type header = (word_type(get_header()) << (word_width - header_width));
	return header | m_data;
}

template <
    size_t HeaderAlignment,
    typename SubwordType,
    typename PhywordType,
    typename Dictionary,
    typename Instruction>
constexpr typename UTMessage<HeaderAlignment, SubwordType, PhywordType, Dictionary, Instruction>::
    header_type
    UTMessage<HeaderAlignment, SubwordType, PhywordType, Dictionary, Instruction>::get_header()
{
	return hate::index_type_list_by_type<Instruction, Dictionary>::value;
}

template <
    size_t HeaderAlignment,
    typename SubwordType,
    typename PhywordType,
    typename Dictionary,
    typename Instruction>
constexpr typename UTMessage<HeaderAlignment, SubwordType, PhywordType, Dictionary, Instruction>::
    payload_type
    UTMessage<HeaderAlignment, SubwordType, PhywordType, Dictionary, Instruction>::get_payload()
        const
{
	return m_data;
}

template <
    size_t HeaderAlignment,
    typename SubwordType,
    typename PhywordType,
    typename Dictionary,
    typename Instruction>
constexpr void
UTMessage<HeaderAlignment, SubwordType, PhywordType, Dictionary, Instruction>::set_payload(
    payload_type const& value)
{
	m_data = value;
}

template <
    size_t HeaderAlignment,
    typename SubwordType,
    typename PhywordType,
    typename Dictionary,
    typename Instruction>
constexpr typename Instruction::Payload
UTMessage<HeaderAlignment, SubwordType, PhywordType, Dictionary, Instruction>::decode() const
{
	typename Instruction::Payload payload;
	payload.decode(get_payload());
	return payload;
}

template <
    size_t HeaderAlignment,
    typename SubwordType,
    typename PhywordType,
    typename Dictionary,
    typename Instruction>
constexpr void
UTMessage<HeaderAlignment, SubwordType, PhywordType, Dictionary, Instruction>::encode(
    typename Instruction::Payload const& value)
{
	set_payload(value.template encode<SubwordType>());
}

template <
    size_t HeaderAlignment,
    typename SubwordType,
    typename PhywordType,
    typename Dictionary,
    typename Instruction>
bool UTMessage<HeaderAlignment, SubwordType, PhywordType, Dictionary, Instruction>::operator==(
    UTMessage<HeaderAlignment, SubwordType, PhywordType, Dictionary, Instruction> const& other)
    const
{
	return (m_data == other.m_data);
}

template <
    size_t HeaderAlignment,
    typename SubwordType,
    typename PhywordType,
    typename Dictionary,
    typename Instruction>
bool UTMessage<HeaderAlignment, SubwordType, PhywordType, Dictionary, Instruction>::operator!=(
    UTMessage<HeaderAlignment, SubwordType, PhywordType, Dictionary, Instruction> const& other)
    const
{
	return !(*this == other);
}

template <
    size_t HeaderAlignment,
    typename SubwordType,
    typename PhywordType,
    typename Dictionary,
    typename Instruction>
std::ostream& operator<<(
    std::ostream& os,
    UTMessage<HeaderAlignment, SubwordType, PhywordType, Dictionary, Instruction> const& message)
{
	os << "UTMessage(";
	auto const words = message.get_raw().to_array();
	for (auto iter = words.rbegin(); iter != words.rend(); iter++) {
		std::stringstream ss;
		ss << std::hex << std::setfill('0') << std::setw(message.subword_width / 4)
		   << static_cast<largest_ut_message_subword_type>(*iter);
		os << ss.str();
	}
	os << ")";
	return os;
}

template <
    size_t HeaderAlignment,
    typename SubwordType,
    typename PhywordType,
    typename Dictionary,
    typename Instruction>
template <typename Archive>
void UTMessage<HeaderAlignment, SubwordType, PhywordType, Dictionary, Instruction>::cerealize(
    Archive& ar)
{
	ar(CEREAL_NVP(m_data));
}

} // namespace hxcomm
