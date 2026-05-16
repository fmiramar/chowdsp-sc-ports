#pragma once

namespace chowdsp
{

template <typename T>
inline int signum(T val)
{
    return (T(0) < val) - (val < T(0));
}

} // namespace chowdsp
