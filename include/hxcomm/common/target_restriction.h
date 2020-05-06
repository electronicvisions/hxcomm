#pragma once
#include <array>

namespace hxcomm {

/**
 * Target restriction describing that given object which is annotated supports only a specified
 * target for e.g. execution.
 */
enum class TargetRestriction
{
	hardware,
	simulation,
	SIZE // Only to be used for iteration
};

constexpr static auto all_target_restrictions = []() {
	std::array<TargetRestriction, static_cast<size_t>(TargetRestriction::SIZE)> ret{};
	for (size_t i = 0; i < ret.size(); ++i) {
		ret[i] = static_cast<TargetRestriction>(i);
	}
	return ret;
}();

} // namespace hxcomm
