
// Build configuration

// Game_Music_Box 0.5.2. Copyright (C) 2005 Shay Green. GNU LGPL license.

#ifndef CONFIG_H
#define CONFIG_H

// Build options

// Disable asserts and other debugging code
//#define NDEBUG

// Enable optimizer (full optimization is enabled only if NDEBUG is also defined)
//#define OPTIMIZE 1

// Break into debugger at debug_out statements
#define DEBUG_OUT_BREAKS 1


// Make use of unrar library
//#define UNRAR_AVAILABLE 1

// Allow non-portable optimizations in Game_Music_Emu
#define BLARGG_NONPORTABLE 1

// Game_Music_Emu uses #include BLARGG_ENABLE_OPTIMIZER for performance-critical functions
#define BLARGG_ENABLE_OPTIMIZER "enable_optimizer.h"

#define BLARGG_SOURCE_BEGIN "source_begin.h"

#define PROGRAM_NAME "Game Music Box"

// Report errors using exceptions
void throw_error( const char* );
#define RAISE_ERROR( str ) throw_error( str )

#ifndef NDEBUG
	#define NSF_EMU_EXTRA_FLAGS 1
#endif

#include "config_compiler.h"

#endif

