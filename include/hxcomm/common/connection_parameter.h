#pragma once
#include <stddef.h>

namespace hxcomm {

/**
 * Set of template parameters common to a UTMessage set.
 * @tparam HeaderAlignmentT Alignment of header in bits
 * @tparam SubwordTypeT Type of subword which's width corresponds to the message's alignment
 * @tparam PhywordTypeT Type of PHY-word which's width corresponds to the message's minimal width
 * @tparam DictionaryT Dictionary of instructions
 */
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


/**
 * Set of template parameters of a connection with different UTMessage sets for sending and
 * receiving direction.
 * @tparam SendHeaderAlignment Alignment of header of send UTMessage set
 * @tparam SendSubwordType Type of subword of send UTMessage set
 * @tparam SendPhywordType Type of PHY-word of send UTMessage set
 * @tparam SendDictionary Dictionary of instructions of sent UTMessages
 * @tparam SendHaltInstructionType Instruction type of Halt message in sent diciontary
 * @tparam ReceiveHeaderAlignment Alignment of header of receive UTMessage set
 * @tparam ReceiveSubwordType Type of subword of receive UTMessage set
 * @tparam ReceivePhywordType Type of PHY-word of receive UTMessage set
 * @tparam ReceiveDictionary Dictionary of instructions of received UTMessages
 * @tparam ReceiveHaltInstructionType Instruction type of Halt message in receive dictionary
 */
template <
    size_t SendHeaderAlignment,
    typename SendSubwordType,
    typename SendPhywordType,
    typename SendDictionary,
    typename SendHaltInstructionType,
    size_t ReceiveHeaderAlignment,
    typename ReceiveSubwordType,
    typename ReceivePhywordType,
    typename ReceiveDictionary,
    typename ReceiveHaltInstructionType,
    typename ReceiveTimeoutInstructionType>
struct ConnectionParameter
{
	typedef UTMessageParameter<
	    SendHeaderAlignment,
	    SendSubwordType,
	    SendPhywordType,
	    SendDictionary>
	    Send;
	typedef SendHaltInstructionType SendHalt;
	typedef UTMessageParameter<
	    ReceiveHeaderAlignment,
	    ReceiveSubwordType,
	    ReceivePhywordType,
	    ReceiveDictionary>
	    Receive;
	typedef ReceiveHaltInstructionType ReceiveHalt;
	typedef ReceiveTimeoutInstructionType ReceiveTimeout;
};

} // namespace hxcomm
