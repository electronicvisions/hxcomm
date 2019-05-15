#include <iomanip>
#include <boost/variant.hpp>

namespace hxcomm {

template <size_t HeaderAlignment, typename SubwordType, typename PhywordType, typename... Is>
struct to_ut_message_variant<HeaderAlignment, SubwordType, PhywordType, hate::type_list<Is...> >
{
	typedef boost::variant<ut_message<HeaderAlignment, SubwordType, PhywordType, hate::type_list<Is...>, Is>...>
	    type;
};


template <size_t HeaderAlignment, typename SubwordType, typename PhywordType, typename... Is>
struct largest_ut_message_size<HeaderAlignment, SubwordType, PhywordType, hate::type_list<Is...> >
{
	static constexpr size_t value = std::max(
	    {ut_message<HeaderAlignment, SubwordType, PhywordType, hate::type_list<Is...>, Is>::word_width...});
};


template <size_t HeaderAlignment, typename Dictionary>
struct ut_message_header_width
{
	static constexpr size_t value = hate::math::round_up_to_multiple(
	    1 /* comma width */ +
	        hate::math::num_bits(
	            hate::type_list_size<Dictionary>::value - 1 /* max. header index */),
	    HeaderAlignment);
};


template <size_t HeaderAlignment, typename SubwordType, typename PhywordType, typename Dictionary, typename Instruction>
constexpr ut_message<HeaderAlignment, SubwordType, PhywordType, Dictionary, Instruction>::ut_message() : m_data()
{}

template <size_t HeaderAlignment, typename SubwordType, typename PhywordType, typename Dictionary, typename Instruction>
constexpr ut_message<HeaderAlignment, SubwordType, PhywordType, Dictionary, Instruction>::ut_message(
    payload_type const& payload) :
    m_data(payload)
{}

template <size_t HeaderAlignment, typename SubwordType, typename PhywordType, typename Dictionary, typename Instruction>
constexpr ut_message<HeaderAlignment, SubwordType, PhywordType, Dictionary, Instruction>::ut_message(
    typename Instruction::payload_type const& payload) :
    m_data(payload.template encode<SubwordType>())
{}

template <size_t HeaderAlignment, typename SubwordType, typename PhywordType, typename Dictionary, typename Instruction>
constexpr typename ut_message<HeaderAlignment, SubwordType, PhywordType, Dictionary, Instruction>::word_type
ut_message<HeaderAlignment, SubwordType, PhywordType, Dictionary, Instruction>::get_raw() const
{
	constexpr word_type header = (word_type(get_header()) << (word_width - header_width));
	return header | m_data;
}

template <size_t HeaderAlignment, typename SubwordType, typename PhywordType, typename Dictionary, typename Instruction>
constexpr typename ut_message<HeaderAlignment, SubwordType, PhywordType, Dictionary, Instruction>::header_type
ut_message<HeaderAlignment, SubwordType, PhywordType, Dictionary, Instruction>::get_header()
{
	return hate::index_type_list_by_type<Instruction, Dictionary>::value;
}

template <size_t HeaderAlignment, typename SubwordType, typename PhywordType, typename Dictionary, typename Instruction>
constexpr typename ut_message<HeaderAlignment, SubwordType, PhywordType, Dictionary, Instruction>::payload_type
ut_message<HeaderAlignment, SubwordType, PhywordType, Dictionary, Instruction>::get_payload() const
{
	return m_data;
}

template <size_t HeaderAlignment, typename SubwordType, typename PhywordType, typename Dictionary, typename Instruction>
constexpr void ut_message<HeaderAlignment, SubwordType, PhywordType, Dictionary, Instruction>::set_payload(
    payload_type const& value)
{
	m_data = value;
}

template <size_t HeaderAlignment, typename SubwordType, typename PhywordType, typename Dictionary, typename Instruction>
constexpr typename Instruction::payload_type
ut_message<HeaderAlignment, SubwordType, PhywordType, Dictionary, Instruction>::decode() const
{
	typename Instruction::payload_type payload;
	payload.decode(get_payload());
	return payload;
}

template <size_t HeaderAlignment, typename SubwordType, typename PhywordType, typename Dictionary, typename Instruction>
constexpr void ut_message<HeaderAlignment, SubwordType, PhywordType, Dictionary, Instruction>::encode(
    typename Instruction::payload_type const& value)
{
	set_payload(value.template encode<SubwordType>());
}

template <size_t HeaderAlignment, typename SubwordType, typename PhywordType, typename Dictionary, typename Instruction>
bool ut_message<HeaderAlignment, SubwordType, PhywordType, Dictionary, Instruction>::operator==(
    ut_message<HeaderAlignment, SubwordType, PhywordType, Dictionary, Instruction> const& other) const
{
	return (m_data == other.m_data);
}

template <size_t HeaderAlignment, typename SubwordType, typename PhywordType, typename Dictionary, typename Instruction>
bool ut_message<HeaderAlignment, SubwordType, PhywordType, Dictionary, Instruction>::operator!=(
    ut_message<HeaderAlignment, SubwordType, PhywordType, Dictionary, Instruction> const& other) const
{
	return !(*this == other);
}

template <size_t HeaderAlignment, typename SubwordType, typename PhywordType, typename Dictionary, typename Instruction>
std::ostream& operator<<(
    std::ostream& os,
    ut_message<HeaderAlignment, SubwordType, PhywordType, Dictionary, Instruction> const& message)
{
	os << "ut_message(";
	auto const words = message.get_raw().to_array();
	for (auto iter = words.rbegin(); iter != words.rend(); iter++) {
		std::stringstream ss;
		ss << std::hex << std::setfill('0') << std::setw(message.subword_width / 4) << *iter;
		os << ss.str();
	}
	os << ")";
	return os;
}

template <size_t HeaderAlignment, typename SubwordType, typename PhywordType, typename Dictionary, typename Instruction>
template <typename Archive>
void ut_message<HeaderAlignment, SubwordType, PhywordType, Dictionary, Instruction>::cerealize(Archive& ar)
{
	ar(CEREAL_NVP(m_data));
}

} // namespace hxcomm
