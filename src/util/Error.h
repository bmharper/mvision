#pragma once

namespace sx
{

//#define SX_BREAKME (void)0
#ifndef SX_BREAKME
#	ifdef _MSC_VER
#		define SX_BREAKME __debugbreak()
#	else
#		define SX_BREAKME __builtin_trap()
#	endif
#endif

// This is compiled in all builds (debug, release, etc)
#define SXASSERT(exp) (void) ((exp) || (printf("Error: %s:%d:\n %s\n", __FILE__, __LINE__, #exp), SX_BREAKME, exit(1), 0))
#define SXDIE_OOM() (void) (printf("Out of memory: %s:%d:\n", __FILE__, __LINE__), SX_BREAKME, exit(1), 0)

}
