
// Debugging related. Will probably require modification for other environments.

// Game_Music_Box 0.5.2. http://www.slack.net/~ant/game-music-box

#include "common.h"

#include <cstdarg>
#include <cstdlib>
#include <cstdio>
//#include <DriverServices.h>

#include "mac_util.h"
#include "file_util.h"

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

void flush_debug_log();

#if !OPTIMIZE && !defined (NDEBUG)
	// uncomment to break into debugger when an exception is thrown (requires modified
	// C++ runtime)
	//#define BREAK_ON_THROW 1
#endif

// Overrides for CodeWarrior

#ifdef __MWERKS__

// Log assertion failure
#include <cassert>
extern "C" void __assertion_failed( char* expr, char* file, int line );
extern "C" void __assertion_failed( char* expr, char* file, int line )
{
	char str [256];
	std::sprintf( str, "Assertion failed, file \"%s\", line %d", file, line, expr );
	debug_out << str;
	
	report_error( "An internal error occurred (assertion failure). Sorry." );
	
	#ifndef NDEBUG
		flush_debug_log();
	#endif
	std::abort();
}

// Avoid Carbon-incompatible glue MWCW library uses
extern "C" int __system7present();
extern "C" int __system7present() {
	return true;
}

// Break when C++ exception is thrown
#if BREAK_ON_THROW

	#include "MWCW_set_throw_hook"
	
	static void throw_handler( char const* type, void* obj, void* dtor )
	{
		if ( type )
			debug_out << "exception thrown";
		__std_throw_handler( type, obj, dtor );
	}

	static int i = (__set_throw_hook( throw_handler ), 0);

#endif

// Write console output to log file
#include <cstdio>
#include <console.h>

short InstallConsole( short fd ) {
	return 0;
}

void RemoveConsole() {
}

long WriteCharsToConsole( char* p, long n )
{
	if ( n )
	{
		try
		{
			static int log_exists;
			static FSRef log_path;
			if ( !log_exists ) {
				FSRef dir = FindFolderChk( kDesktopFolderType );
				HFSUniStr255 name;
				str_to_filename( "debug.txt", name );
				if ( !FSMakeFSRefExists( dir, name, &log_path ) )
					log_path = create_file( dir, name, 'TEXT' );
			}
			
			Mac_File file( log_path );
			file.seek( file.size() );
			if ( !log_exists ) {
				log_exists = true;
				file.write( "----\r", 5 );
			}
			
			file.write( p, n );
		}
		catch ( ... ) {
		}
	}
	return n;
}

long ReadCharsFromConsole( char*, long ) {
	return 0;
}

char* __ttyname( long fd ) {
	return (0 <= fd && fd <= 2) ? "null device" : NULL;
}

#endif

#ifndef NDEBUG

const long debug_max = 16 * 1024L;

static char debug_buffer [debug_max];
static SInt32 debug_size;

// thread-safe
static void debug_write( const char* str )
{
	const int len = std::strlen( str );
	long pos = AddAtomic( len, &debug_size );
	if ( pos + len > debug_max )
		debug_size = debug_max;
	
	long n = debug_max - pos;
	if ( n > len )
		n = len;
	if ( n > 0 )
		std::memcpy( debug_buffer + pos, str, n );
}

void flush_debug_log()
{
	if ( debug_size )
	{
		static int log_exists;
		static FSRef log_path;
		if ( !log_exists ) {
			FSRef dir = FindFolderChk( kDesktopFolderType );
			HFSUniStr255 name;
			str_to_filename( "debug.txt", name );
			if ( !FSMakeFSRefExists( dir, name, &log_path ) )
				log_path = create_file( dir, name, 'TEXT' );
		}
		
		Mac_File file( log_path );
		file.set_size( 0 );
		/*
		file.seek( file.size() );
		if ( !log_exists ) {
			log_exists = true;
			file.write( "----\r", 5 );
		}
		*/
		
		file.write( debug_buffer, debug_size );
		if ( debug_size == debug_max ) {
			static const char str [] = "\nDebug buffer overflowed\n";
			file.write( str, sizeof str - 1 );
		}
	}
}

struct flush_debug_log_ {
	~flush_debug_log_() {
		flush_debug_log();
	}
};

static flush_debug_log_ flush_debug_logx;

static void debug_log( const char* file, int line, const char* fmt, std::va_list args )
{
	static UInt32 active;
	static bool collision;
	
	// to do: allow fully concurrent operation
	if ( BitOrAtomic( 1, &active ) ) {
		collision = true;
		return;
	}
	
	char str [256];
	
	int len = std::sprintf( str, "%s line %d\t", file, line );
	std::vsprintf( str + len, fmt, args );
	debug_write( str );
	
	if ( collision ) {
		collision = false;
		debug_write( "Debug message lost\n" );
	}
	assert( active );
	active = false;
}

void Debug_Printf::operator () ( const char* fmt, ... )
{
	std::va_list args;
	va_start( args, fmt );
	debug_log( file, line, fmt, args );
	va_end( args );
}

void check_failed( const char* file, int line, const char* expr )
{
	Debug_Printf( file, line )( "Check failed\n" );
}

#endif

