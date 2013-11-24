
// Game_Music_Box 0.5.2. http://www.slack.net/~ant/game-music-box

#include "util.h"

#include <ctime>
//#include <TextUtils.h>

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

void pstrcpy( unsigned char* out, const unsigned char* in )
{
	std::memcpy( out, in, in [0] + 1 );
}

void num_to_str( long n, char* out, int leading ) 
{
    char buf [32];
	snprintf( &buf[0], sizeof(buf), "%ld", n );
	char lch = ' ';
	if ( leading < 0 ) {
		leading = -leading;
		lch = '0';
	}
	leading -= buf [0];
	while ( --leading >= 0 )
		*out++ = lch;
	strcpy( out, buf );
}

void cstr_to_hfs( const char* const pszCString, HFSUniStr255& hfsString )
{
    int len = strlen(pszCString);
    hfsString.length = len;
    for (int i=0; i<len; i++)
    {
        hfsString.unicode[i] = pszCString[i];
    }
}

int random( int range )
{
	static int init = (std::srand( (int) std::clock() % RAND_MAX ),0);
	return long (std::rand()) * range / (RAND_MAX + 1L);
}

char* str_end( char* p )
{
	while ( *p )
		++p;
	return p;
}

void strcpy_trunc( char* out, const char* in, int out_size )
{
	require( out_size > 0 );
	out [out_size - 1] = 0;
	std::strncpy( out, in, out_size - 1 );
}

