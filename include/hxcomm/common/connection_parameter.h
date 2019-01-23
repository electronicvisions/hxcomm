#pragma once
#include <stddef.h>

namespace hxcomm {

template <size_t HeaderAlignmentT, typename SubwordTypeT, typename DictionaryT>
struct UTMessageParameter
{
	static constexpr size_t HeaderAlignment = HeaderAlignmentT;
	typedef SubwordTypeT SubwordType;
	typedef DictionaryT Dictionary;
};

template <
    size_t SendHeaderAlignment,
    typename SendSubwordType,
    typename SendDictionary,
    size_t ReceiveHeaderAlignment,
    typename ReceiveSubwordType,
    typename ReceiveDictionary,
    typename ReceiveHaltInstructionType>
struct ConnectionParameter
{
	typedef UTMessageParameter<SendHeaderAlignment, SendSubwordType, SendDictionary> Send;
	typedef UTMessageParameter<ReceiveHeaderAlignment, ReceiveSubwordType, ReceiveDictionary>
	    Receive;
	typedef ReceiveHaltInstructionType ReceiveHalt;
};

} // namespace hxcomm
