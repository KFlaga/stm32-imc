#pragma once

#include <cstdint>

#define CONCAT_IMPL( x, y ) x##y
#define MACRO_CONCAT( x, y ) CONCAT_IMPL( x, y )
#define CONCAT3_IMPL( x, y, z ) x##y##z
#define MACRO_CONCAT3( x, y, z ) CONCAT3_IMPL( x, y, z )
#define DEFER_UNIQUE_NAME( name ) MACRO_CONCAT( name, __COUNTER__ )
#define MAKE_UNIQUE_NAME( name ) DEFER_UNIQUE_NAME( name )

#define PADDING(n) std::uint8_t MAKE_UNIQUE_NAME(_pad) [n] = {}
