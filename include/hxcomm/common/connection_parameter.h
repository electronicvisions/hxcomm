#pragma once
#include <stddef.h>

namespace hxcomm {

template <
    size_t HeaderAlignmentT,
    typename SubwordTypeT,
    typename PhywordTypeT,
    typename DictionaryT>
struct UTMessageParameter
{
	static constexpr size_t HeaderAlignment = HeaderAlignmentT;
	typedef SubwordTypeT SubwordType;
	typedef PhywordTypeT PhywordType;
	typedef DictionaryT Dictionary;
};

template <
    size_t SendHeaderAlignment,
    typename SendSubwordType,
    typename SendPhywordType,
    typename SendDictionary,
    size_t ReceiveHeaderAlignment,
    typename ReceiveSubwordType,
    typename ReceivePhywordType,
    typename ReceiveDictionary,
    typename ReceiveHaltInstructionType>
struct ConnectionParameter
{
	typedef UTMessageParameter<
	    SendHeaderAlignment,
	    SendSubwordType,
	    SendPhywordType,
	    SendDictionary>
	    Send;
	typedef UTMessageParameter<
	    ReceiveHeaderAlignment,
	    ReceiveSubwordType,
	    ReceivePhywordType,
	    ReceiveDictionary>
	    Receive;
	typedef ReceiveHaltInstructionType ReceiveHalt;
};

} // namespace hxcomm
