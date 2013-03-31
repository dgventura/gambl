
// Game_Music_Box 0.5.2. http://www.slack.net/~ant/game-music-box

#include "debug_out.h"
#include "common.h"

// std
#include <cstdlib>
#include <cstdio>

// MacOS
//#include <NumberFormatting.h>
//#include <Memory.h>

/* Copyright (C) 2005 Shay Green. This module is free software; you can
redistribute it and/or modify it under the terms of the GNU Lesser General
Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
module is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details. You should have received a copy of the GNU Lesser General
Public License along with this module; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA */

static unsigned char str [512];
static std::size_t len;

unsigned char const* debug_out_::output_end()
{
	#ifdef NDEBUG
	{
		len = 0;
	}
	#else
	{
		// don't write to file if debug breaks are enabled
		#if DEBUG_OUT_BREAKS
		{
			// convert newlines
			for ( unsigned char* p = str + len; p != str; --p )
				if ( *p == '\n' )
					*p = '\r';
		}
		#else
		{
			// write to standard output
			std::fwrite( &str [1], len, 1, stdout );
			std::putchar( '\n' );
			std::fflush( stdout );
		}
		#endif
	}
	#endif
	
	// make pascal string for Debugger()
	str [0] = len;
	len = 0;
	return str;
}

void debug_out_::append( void const* p )
{
	static char const hex_digits [] = "0123456789ABCDEF";
	
	append( "0x" );
	--len;
	
	for ( int i = 0; i < 8; ++i )
		str [len + 1 + 7 - i] = hex_digits [(reinterpret_cast<unsigned long> (p) >> (i * 4)) & 0x0F];
	
	len += 8;
}

void debug_out_::append( char const* s )
{
	unsigned char* p = str + len + 1;
	while ( char c = *s++ )
		*p++ = c;
	*p++ = ' ';
	len = p - str - 1;
}

void debug_out_::append( unsigned char const* s )
{
	unsigned char* p = str + len + 1;
	for ( int i = *s++; i--; )
		*p++ = *s++;
	*p++ = ' ';
	len = p - str - 1;
}

void debug_out_::append( int n )
{
	unsigned char s [32];
	NumToString( n, s );
	append( s );
}

void debug_out_::append_double( double n )
{
	char str [32];
	str [0] = std::sprintf( str + 1, "%f", n );
	append( (unsigned char*) str );
}

