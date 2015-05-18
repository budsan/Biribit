#pragma once

#include <memory>
#include <cstdint>

template<class T> using shared = std::shared_ptr < T >;
template<class T> using unique = std::unique_ptr < T >;

enum PacketReliabilityBitmask
{
	Unreliable = 0,
	Reliable = 1,
	Ordered = 2,
	ReliableOrdered = 3
};
