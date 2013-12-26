//
//  FileUtilities.cpp
//  GaMBL
//
//  Created by David Ventura on 12/25/13.
//  Copyright (c) 2013 David Ventura. All rights reserved.
//

#include "FileUtilities.h"

// Mac_File

Mac_File_Reader::Mac_File_Reader( const std::wstring& file, short perm ) :
fsref( file ),
fsref_valid( true )
{
	unclosed = true;
	mode = 0;
    assert ( fsref.IsOk() );
	//throw_error( status );
}

Mac_File_Reader::Mac_File_Reader( short ref_ )
{
	ref = ref_;
	unclosed = true;
	fsref_valid = false;
	mode = 0;
}

Mac_File_Reader::~Mac_File_Reader()
{
	close();
}

long Mac_File_Reader::size()
{
	return fsref.size();
}

void Mac_File::set_size( long s )
{
	int result = ftruncate( fsref.FileDescriptor(), s );
    assert( result );
}

long Mac_File_Reader::tell()
{
	SInt64 pos;
	throw_error( DeprecatedFSGetForkPosition( ref, &pos ) );
	return pos;
}

Mac_File_Reader::error_t Mac_File_Reader::seek( long n )
{
	throw_error( DeprecatedFSSetForkPosition( ref, fsFromStart, n ) );
	return NULL;
}

Mac_File_Reader::error_t Mac_File_Reader::read( void* p, long s )
{
	if ( read_avail( p, s ) != s )
		throw_error( eofErr );
	return NULL;
}

long Mac_File_Reader::read_avail( void* p, long s )
{
	ByteCount count = 0;
	throw_unless( DeprecatedFSReadFork( ref, fsAtMark + mode, 0, s, p, &count ), eofErr );
	return count;
}

void Mac_File_Reader::close()
{
	if ( unclosed ) {
		unclosed = false;
//RAD		debug_if_error( DeprecatedFSCloseFork( ref ) );
	}
	ref = -1;
}

void Mac_File_Reader::throw_error( long err )
{
	if ( !err )
		return;
/*	if ( !fsref_valid && !FSGetForkCBInfo( ref, 0, NULL, NULL, NULL, &fsref, NULL ) )
		fsref_valid = true;
	if ( fsref_valid )
		throw_file_error( err, fsref );
 */
	else
		throw_error( err );
}

Data_Writer::error_t Mac_File::write( const void* p, long s )
{
	ByteCount count;
	throw_error( DeprecatedFSWriteFork( ref, fsAtMark + mode, 0, s, p, &count ) );
	if ( count != s )
		throw_error( ioErr );
	return NULL;
}

void Mac_File_Reader::set_cached( bool b ) {
	mode = (b ? 0 : noCacheMask);
}
Mac_File::Mac_File( const std::wstring& r ) : Mac_File_Reader( r, fsRdWrPerm ) {
}
Mac_File::Mac_File( short r ) : Mac_File_Reader( r ) {
}


bool AreFilesEqual( const std::wstring& strPath1, const std::wstring& strPath2 ) const
{
    assert( 0 );
    
    //todo resolve paths to final files and see if they're the same
}

bool has_extension( const char* str, const char* suffix )
{
	int len = std::strlen( suffix );
	int strl = std::strlen( str );
	if ( strl <= len )
		return false;
	
	for ( int i = 0; i < len; i++ )
		if ( std::toupper( str [strl - len + i] ) != std::toupper( suffix [i] ) )
			return false;
	
	return true;
}