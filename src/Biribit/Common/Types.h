#pragma once

#include <memory>
#include <cstdint>

template<class T> using shared = std::shared_ptr < T >;
template<class T> using unique = std::unique_ptr < T >;

