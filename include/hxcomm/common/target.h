#pragma once

namespace hxcomm {

/**
 * Target describing that given object which is annotated supports a specified
 * target for e.g. execution.
 */
enum class Target
{
	hardware,
	self_contained_pbmem,
	simulation,
};

} // namespace hxcomm
