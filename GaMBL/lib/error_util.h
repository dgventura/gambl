
// Exception classes and utilities to throw them

// Game_Music_Box 0.5.2. Copyright (C) 2005 Shay Green. GNU LGPL license.

#ifndef ERROR_UTIL_H
#define ERROR_UTIL_H

#include "common.h"

#include <exception>

#define EXIT_IF_ERROR( expr ) if ( (err = (expr)) != 0 ) goto exit;

struct Error_Code : std::exception {
	long code;
	Error_Code( long n ) : code( n ) { }
};

struct Error_Message : Error_Code {
	const char* str;
	Error_Message( const char* s, long n = -1 ) : Error_Code( n ), str( s ) { }
	const char* what() const throw ();
};

struct File_Error : Error_Message {
	FSRef fsref;
	File_Error( long n, const FSRef& r, const char* str = NULL ) :
			Error_Message( str, n ), fsref( r ) { }
};

const int exception_str_max = 1024;
const char* error_to_str( long code );
bool exception_to_str( char* out );
long exception_to_code();

// to do:
// bool throw_if_other_error( error, compare ) {
//     if ( error == compare ) return true;
//     throw_if_error( error );
//     return false;
// }

void throw_null();
void throw_error( long code );
void throw_error( const char* str );

// Throw error unless equal to non_error. Returns true if error was non_error,
// false if error was 0 (no error).
bool throw_unless( long code, long non_error );
bool throw_unless( const char* str, const char* non_error );

void throw_file_error( const char*, const FSRef& );
void throw_file_error( long code, const FSRef& );
void throw_file_error( long code, const FSRef&, const HFSUniStr255& );

template<class T>
inline void throw_if_error( T err )
{
	if ( err ) {
		throw_error( err );
		while ( true ) { } // help optimizer
	}
}

template<class T>
inline T throw_if_null( T t )
{
	if ( !t ) {
		throw_null();
		while ( true ) { } // help optimizer
	}
	return t;
}

long debug_if_error_( long );

inline long debug_if_error( long err )
{
	#ifndef NDEBUG
		if ( err )
			return debug_if_error_( err );
	#endif
	return err;
}

template<class T>
inline T debug_if_null( T t )
{
	#ifndef NDEBUG
		if ( !t )
			dprintf( 0, "NULL" );
	#endif
	return t;
}

#endif

