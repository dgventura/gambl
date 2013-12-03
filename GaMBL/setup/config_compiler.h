
// Compiler configuration

// Game_Music_Box 0.5.2. Copyright (C) 2005 Shay Green. GNU LGPL license.

#ifndef CONFIG_COMPILER_H
#define CONFIG_COMPILER_H

// Build options to be set before #including this file.

// Disable asserts and other debugging code
//define NDEBUG

// Enable optimizer (full optimization is enabled only if NDEBUG is also defined)
//define OPTIMIZE 1

// Disable debugging symbols when NDEBUG is defined
//define NOSYMBOLS 1

// CodeWarrior configuration
#ifdef __MWERKS__
	
	// fix const-correctness 
	#define __lhbrx( addr, offset ) \
		__lhbrx( const_cast<void*> (static_cast<const void*> (addr)), offset )
	
	// configure optimizer
	#if !OPTIMIZE
		// all optimization off to help debugger
		#pragma peephole off
		#pragma global_optimizer off
		#pragma optimization_level 0
		#pragma scheduling off
	
	#elif !defined (NDEBUG)
		// basic optimization is better than none
		#pragma optimization_level 1
		#pragma scheduling 750
	#endif
	
	#if defined (NDEBUG) && NOSYMBOLS
		// function names for debugger to see
		#pragma traceback off
	#endif
#endif

#endif

