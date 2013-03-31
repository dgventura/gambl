
// Game_Music_Box 0.5.2. http://www.slack.net/~ant/game-music-box

#include "mac_memory.h"

/* Copyright (C) 2005 Shay Green. This module is free software; you
can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
library is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
more details. You should have received a copy of the GNU Lesser General
Public License along with this module; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA */

#include "source_begin.h"
 
// to do: for out of memory, report how much more was needed
// to do: remove some assertions

void* NewPtrChk( long s )
{
	require( s >= 0 );
	void* p = NewPtr( s );
	assert( !p || !MemError() );
	if ( p )
		return p;
	throw_error( "Out of memory" );
	while ( true ) { }
}

long GetPtrSizeChk( void* p )
{
	require( IsPointerValid( static_cast<Ptr> (p) ) );
	long s = GetPtrSize( static_cast<Ptr> (p) );
	assert( !MemError() );
	return s;
}

void SetPtrSizeChk( void* p, long s )
{
	require( s >= 0 );
	require( IsPointerValid( static_cast<Ptr> (p) ) );
	SetPtrSize( static_cast<Ptr> (p), s );
	throw_if_error( MemError() );
	assert( GetPtrSizeChk( p ) == s );
}

void DisposePtrChk( void* p )
{
	require( IsPointerValid( static_cast<Ptr> (p) ) );
	DisposePtr( static_cast<Ptr> (p) );
	assert( !MemError() );
}

Handle NewHandleChk( long s )
{
	require( s >= 0 );
	Handle h = NewHandle( s );
	assert( !h || !MemError() );
	if ( h )
		return h;
	throw_error( "Out of memory" );
	while ( true ) { }
}

Handle TempNewHandleNothrow( long s )
{
	require( s >= 0 );
	OSErr err = 0;
	Handle h = TempNewHandle( s, &err );
	assert( !h || !err );
	return h;
}

Handle TempNewHandleChk( long s )
{
	Handle h = TempNewHandleNothrow( s );
	if ( h )
		return h;
	throw_error( "Out of memory" );
	while ( true ) { }
}

long GetHandleSizeChk( Handle h )
{
	require( IsHandleValid( h ) );
	long s = GetHandleSize( h );
	assert( !MemError() );
	return s;
}

void SetHandleSizeChk( Handle h, long s )
{
	require( s >= 0 );
	require( IsHandleValid( h ) );
	SetHandleSize( h, s );
	throw_if_error( MemError() );
	assert( GetHandleSizeChk( h ) == s );
}

Handle RecoverHandleChk( void* p )
{
	require( p );
	Handle h = RecoverHandle( static_cast<Ptr> (p) );
	assert( h && *h == p && !MemError() && IsHandleValid( h ) );
	return h;
}

void DisposeHandleChk( Handle h )
{
	require( IsHandleValid( h ) );
	DisposeHandle( h );
	assert( !MemError() );
}

