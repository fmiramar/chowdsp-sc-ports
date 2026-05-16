#pragma once

#include <cstddef>

namespace xsimd
{
struct default_arch
{
    static constexpr std::size_t alignment() noexcept
    {
        return alignof(float);
    }
};
} // namespace xsimd

#include "../../../../chowdsp_utils/examples/ModalSpringReverb/ModeParams.h"
