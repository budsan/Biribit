#pragma once

#include <memory>

namespace brbt
{
	typedef unsigned int   packet_id_t;
	typedef unsigned short id_t;
	typedef unsigned short str_size_t;
}

template<class T> using shared = std::shared_ptr < T >;
template<class T> using unique = std::unique_ptr < T >;

enum PacketReliabilityBitmask
{
	Unreliable = 0,
	Reliable = 1,
	Ordered = 2,
	ReliableOrdered = 3
};
