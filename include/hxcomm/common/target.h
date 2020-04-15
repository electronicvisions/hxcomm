#pragma once

namespace hxcomm {

/**
 * Target describing that given object which is annotated supports a specified
 * target for e.g. execution.
 */
enum class Target
{
	hardware,
	hardware_non_interactive,
	simulation,
};

} // namespace hxcomm
