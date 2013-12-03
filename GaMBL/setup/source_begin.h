
// Debug support. To be included *last* in *source* files only so that name clashes
// won't be a problem.

#ifndef SOURCE_DEBUG_H
#define SOURCE_DEBUG_H

#include "blargg_source.h"
#include "debug.h"

#undef dprintf
#ifdef NDEBUG
	// use macro rather than inline
	#define dprintf (1) ? ((void) 0) : (void)
#else
	#define dprintf Debug_Printf( __FILE__, __LINE__ )
#endif

#undef DPRINTF
#define DPRINTF dprintf

#undef require
#define require assert

void check_failed( const char* file, int line, const char* expr );

#undef check
#ifdef NDEBUG
	#define check( expr ) ((void) 0)
#else
	#define check( expr ) \
		(void) ((expr) ? void (0) : check_failed( __FILE__, __LINE__, #expr ))
#endif

#endif

