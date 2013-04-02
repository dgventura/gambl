
// Overrides for operator new [] to allocate using temp memory if large allocation
// can't be satisfied in app heap.

// Game_Music_Box 0.5.2. Copyright (C) 2005 Shay Green. GNU LGPL license.

#include "common.h"
#include <memory>
//#include <Gestalt.h>
#include "mac_memory.h"

static std::size_t large_block_min = 0;
static void* app_mem_min;
static void* app_mem_max;

inline void* allocated_in_app( void* p )
{
	if ( p )
	{
		if ( !app_mem_min || p < app_mem_min )
			app_mem_min = p;
		if ( !app_mem_max || p > app_mem_max )
			app_mem_max = p;
	}
	return p;
}

inline bool is_app_allocation( void* p ) {
	return app_mem_min <= p && p <= app_mem_max;
}

static void* alloc_temp( std::size_t s )
{
	Handle h = TempNewHandleNothrow( s );
	if ( !h )
		return NULL;
	HLock( h );
	assert( !is_app_allocation( *h ) );
	return *h;
}

void* operator new [] ( std::size_t s, const std::nothrow_t& )
{
	if ( !large_block_min )
	{
		large_block_min = (std::size_t) -1;
		long response = 0;
		//TODO: version check memory if ( !Gestalt( gestaltSystemVersion, &response ) && response < 0x01000 )
			large_block_min = 64 * 1024L + 0x100;
			//large_block_min = 384 * 1024L + 0x100;
	}
	
	if ( s > large_block_min )
		return alloc_temp( s );
	
	return allocated_in_app( operator new ( s, std::nothrow ) );
}

void* operator new [] ( std::size_t s )
{
	void* p = operator new [] ( s, std::nothrow );
	if ( !p )
		p = allocated_in_app( operator new ( s ) ); // will probably throw
	return p;
}

void operator delete [] ( void* p )
{
	if ( p )
	{
		if ( is_app_allocation( p ) )
			operator delete ( p );
		else
			DisposeHandleChk( RecoverHandleChk( p ) );
	}
}

