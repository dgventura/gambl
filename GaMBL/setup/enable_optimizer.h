
// Enable optimizer for remaining functions in file

// Game_Music_Box 0.5.2. Copyright (C) 2005 Shay Green. GNU LGPL license.

#ifndef ENABLE_OPTIMIZER_H
#define ENABLE_OPTIMIZER_H

// don't turn optimization *down* when doing fully optimized build
#if !OPTIMIZE
	struct avoid_cw_optimizer_bug { };
	#pragma peephole on
	#pragma global_optimizer on
	#pragma optimization_level 1
#endif

#endif

