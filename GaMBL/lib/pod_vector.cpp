
// Game_Music_Box 0.5.2. http://www.slack.net/~ant/game-music-box

#include "pod_vector.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <new>

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

void pod_vector_impl::init( size_type s )
{
	begin = NULL;
	end = NULL;
	size_ = 0;
	capacity_ = 0;
	sizeof_t = s;
}

void pod_vector_impl::destruct()
{
	std::free( begin );
	begin = NULL;
}

void pod_vector_impl::reserve( size_type res )
{
	if ( capacity_ < res )
	{
		char* p = (char*) std::realloc( begin, res * sizeof_t );
		if ( !p )
			throw std::bad_alloc();
		
		begin = p;
		end = p + size_ * sizeof_t;
		capacity_ = res;
	}
}

void pod_vector_impl::resize( size_type s )
{
	size_type cap = capacity_;
	if ( cap < s )
	{
		do {
			// cap = cap * 1.5 + 32
			cap += (cap >> 1) + 32;
		}
		while ( cap < s );
		
		reserve( cap );
	}
	size_ = s;
	end = begin + s * sizeof_t;
}

void pod_vector_impl::check_iter_( void* p )
{
	size_type offset = (char*) p - begin;
	size_type pos = offset / sizeof_t;
	assert( pos <= size_ && pos * sizeof_t == offset );
}

inline void pod_vector_impl::check_iter( void* p )
{
#ifndef NDEBUG
	check_iter_( p );
#endif
}

void* pod_vector_impl::insert( void* p, size_type count )
{
	check_iter( p );
	
	size_type offset = (char*) p - begin;
	size_type remain = end - (char*) p;
	resize( size_ + count );
	char* pp = begin + offset;
	
	std::memmove( pp + count * sizeof_t, pp, remain );
	return pp;
}

void pod_vector_impl::erase( void* first, void* last )
{
	check_iter( first );
	check_iter( last );
	assert( first <= last );
	
	size_type count = ((char*) last - (char*) first) / sizeof_t;
	assert( size_ >= count );
	
	size_ -= count;
	std::memmove( first, last, end - (char*) last );
	end -= count * sizeof_t;
}

void pod_vector_impl::swap_( pod_vector_impl& other )
{
	assert( sizeof_t == other.sizeof_t );
	std::swap( begin, other.begin );
	std::swap( end, other.end );
	std::swap( size_, other.size_ );
	std::swap( capacity_, other.capacity_ );
}

