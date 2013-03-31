
// Game_Music_Box 0.5.2. Copyright (C) 2005 Shay Green. GNU LGPL license.

#ifndef UTIL_H
#define UTIL_H

#include "common.h"

// Base class for classes whose objects can't be copied or assigned to
class noncopyable {
private:
	noncopyable( const noncopyable& );
	noncopyable& operator = ( const noncopyable& );
public:
	noncopyable() { }
};

template<class T>
class scoped_restorer {
	T* obj;
	T value;
public:
	scoped_restorer( T* p ) : obj( p ), value( *p ) { }
	scoped_restorer( T* p, const T& v ) : obj( p ), value( v ) { }
	~scoped_restorer() { *obj = value; }
};

template<class T>
void reset_container( T& t )
{
	t.clear();
	T empty;
	t.swap( empty );
}

// Copy Pascal string
void pstrcpy( unsigned char* out, const unsigned char* in );

// Convert number to string in base 10. If width > 0, pad string at beginning until
// it's 'width' characters long. If width < 0, pad with '0'.
void num_to_str( long n, char* out, int width = 0 );

// Return pointer to terminator at end of C string
char* str_end( char* );

// Copy at most 'out_size - 1' characters from 'in' to 'out' and terminate
// 'out' at 'out_size'.
void strcpy_trunc( char* out, const char* in, int out_size );

// Generate pseudo-random number n, 0 <= n < range
int random( int range );

#endif

