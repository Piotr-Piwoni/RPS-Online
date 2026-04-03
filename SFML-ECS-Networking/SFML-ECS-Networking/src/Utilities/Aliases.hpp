#pragma once
#include <random>

// Type aliases.
namespace Game
{
template<typename T>
using IntDis = std::uniform_int_distribution<T>;

template<typename T>
using RealDis = std::uniform_real_distribution<T>;

using Port = unsigned short;
}
