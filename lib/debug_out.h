
// Debug output using stream-like interface.

// Game_Music_Box 0.5.2. Copyright (C) 2005 Shay Green. GNU LGPL license.

#ifndef DEBUG_OUT_H
#define DEBUG_OUT_H

//#include <MacTypes.h>

class debug_out_ {
	static void append( void const* );
	static void append( char const* );
	static void append( unsigned char const* );
	static void append_double( double );
	static void append( int );
	static unsigned char const* output_end();
public:
	debug_out_& operator << ( void const* p ) {
		append( p );
		return *this;
	}
	debug_out_& operator << ( unsigned char const* s ) {
		append( s );
		return *this;
	}
	debug_out_& operator << ( char const* s ) {
		append( s );
		return *this;
	}
	debug_out_& operator << ( int n ) {
		append( n );
		return *this;
	}
	debug_out_& operator << ( long n ) {
		append( n );
		return *this;
	}
	debug_out_& operator << ( short n ) {
		append( n );
		return *this;
	}
	debug_out_& operator << ( double n ) {
		append_double( n );
		return *this;
	}
	
	debug_out_() {
	}
	
	struct out_end_ {
		void operator = ( debug_out_ const& ) const {
			const unsigned char* str = output_end();
			#if DEBUG_OUT_BREAKS
				DebugStr( str );
			#endif
		}
	};
	friend struct out_end_;
};

#define debug_out   debug_out_::out_end_() = debug_out_()

#endif

