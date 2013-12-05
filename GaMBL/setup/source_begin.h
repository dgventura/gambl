
// Debug support. To be included *last* in *source* files only so that name clashes
// won't be a problem.

#ifndef SOURCE_DEBUG_H
#define SOURCE_DEBUG_H

#include "blargg_source.h"

#undef dprintf
#ifdef NDEBUG
	// use macro rather than inline
	#define dprintf (1) ? ((void) 0) : (void)
#else
	#define dprintf (void)//TODO: restore formatted printf Debug_Printf( __FILE__, __LINE__ )
#endif

#undef DPRINTF
#define DPRINTF dprintf

#undef require
#define require assert

void check_failed( const char* file, int line, const char* expr );

#undef check
#define check( expr ) ((void) 0)

#endif

