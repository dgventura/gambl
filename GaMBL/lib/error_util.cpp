
// Game_Music_Box 0.5.2. http://www.slack.net/~ant/game-music-box

#include "error_util.h"

/* This module copyright (C) 2005 Shay Green. This module is free software;
you can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
module is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details. You should have received a copy of the GNU Lesser General
Public License along with this module; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA */

#include "source_begin.h"

const char* Error_Message::what() const throw ()
{
	return str ? str : "";
}

void throw_error( const char* str )
{
	if ( str )
		throw Error_Message( str );
}

void throw_error( long code )
{
	if ( code )
		throw Error_Code( code );
}

bool throw_unless( long err, long non_error )
{
	if ( err == non_error )
		return true;
	
	throw_error( err );
	return false;
}

bool throw_unless( const char* err, const char* non_error )
{
	if ( err == non_error )
		return true;
	
	throw_error( err );
	return false;
}

void throw_null() {
	throw_error( "null" );
}

void throw_file_error( const char* str, const GaMBLFileHandle& fsref )
{
	if ( str )
		throw File_Error( -1, fsref, str );
}

void throw_file_error( long code, const GaMBLFileHandle& fsref )
{
	if ( code )
		throw File_Error( code, fsref );
}

void throw_file_error( long code, const GaMBLFileHandle& dir, const HFSUniStr255& name )
{
    //RAD
#if 0
	if ( code )
	{
		GaMBLFileHandle fsref;
		if ( !FSMakeFSRefUnicode( &dir, name.length, name.unicode, 0, &fsref ) )
			throw_file_error( code, fsref );
		else
			throw_file_error( code, dir ); // to do: preserve name for display later
	}
#endif
}

const char* error_to_str( long code )
{
	switch ( code )
	{
		case noMemForPictPlaybackErr:
		case mFulErr:
		case cTempMemErr:
		case cNoMemErr:
		case updPixMemErr:
		case notEnoughBufferSpace:
		case memFragErr:
		case appMemFullErr:
		case notEnoughMemoryErr:
		case notEnoughMemoryToGrab:
		case telNoMemErr:
		case dsMemFullErr:
		case cfragNoPrivateMemErr:
		case cfragNoClientMemErr:
		case cfragNoIDsErr:
		case smNewPErr:
		case memFullErr:    return "Out of memory";
		
		case dupFNErr:      return "An item with the same name already exists";
		case wPrErr:        return "Disk is locked";
		case nsvErr:        return "Disk not found";
		case resNotFound:   return "Resource not found";
		case dirNFErr:      return "Folder not found";
		case fnfErr:        return "File not found";
		case ioErr:         return "Disk error";
		case dskFulErr:     return "Disk full";
		case fBsyErr:       return "File is being used by another application";
		case eofErr:        return "Unexpected end-of-file";
	}
	
	return NULL;
}

static void code_to_str( long code, char* out )
{
	if ( code == -1 )
		return;
	
	const char* str = error_to_str( code );
	
	if ( str ) {
		std::strcpy( out, str );
	}
	else {
		std::strcpy( out, "Error " );
		char code_str [32];
		num_to_str( code, code_str );
		std::strcat( out, code_str );
	}
}

bool exception_to_str( std::wstring& out )
{
	bool result = true;
	
    assert(0);
    
#if 0 //TODO DGV
    
	try
	{
		throw;
		result = false;
	}
	catch ( File_Error& e )
	{
		out.assign( e.what(), strlen(e.what()) );
		code_to_str( e.code, out + out.length() );
		int len = std::strlen( out );
		std::strcat( out, ": " );
		OSErr err = GetFilePath( e.fsref, out );
		if ( err )
			out [len] = 0;
	}
	catch ( Error_Message& e )
	{
		std::strcpy( out, e.what() );
	}
	catch ( Error_Code& e )
	{
		code_to_str( e.code, out );
	}
	catch ( std::bad_alloc& )
	{
		// bypass standard library's cryptic (to an end-user) "Allocation failure"
		std::strcpy( out, "Out of memory" );
	}
	catch ( std::exception& e )
	{
		std::strcpy( out, e.what() );
	}
	catch ( const char* str )
	{
		std::strcpy( out, str );
	}
	catch ( ... )
	{
		result = false;
	}
#endif
    
	return result;
}

long exception_to_code()
{
	long code = -1;
	try {
		throw;
	}
	catch ( Error_Code& e ) {
		code = e.code;
	}
	catch ( ... ) {
	}
	
	return code;
}

long debug_if_error_( long err )
{
	if ( err )
		dprintf( "Error %ld\n", (long) err );
	return err;
}

