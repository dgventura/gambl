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
fsref_valid( true ), fsref( file, "r" )
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
	return fsref.GetSize();
}

void Mac_File::set_size( long s )
{
	int result = ftruncate( fsref.GetDescriptor(), s );
    assert( result );
}

long Mac_File_Reader::tell()
{
	SInt64 pos;
	throw_error( fsref.Tell( &pos ) );
	return pos;
}

Mac_File_Reader::error_t Mac_File_Reader::seek( long n )
{
	throw_error( fsref.Seek( n ) );
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
	ByteCount count = fsref.ReadBytes( p, s );
	//RADthrow_unless( FSReadFork( ref, fsAtMark + mode, 0, s, p, &count ), eofErr );
        
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
    assert(0);
    
    //TODO RAD use POSIX write
/*	ByteCount count;
	throw_error( FSWriteFork( ref, fsAtMark + mode, 0, s, p, &count ) );
	if ( count != s )
		throw_error( ioErr );
 */
	return NULL;
}

void Mac_File_Reader::set_cached( bool b ) {
	mode = (b ? 0 : noCacheMask);
}
Mac_File::Mac_File( const std::wstring& r ) : Mac_File_Reader( r, fsRdWrPerm ) {
}
Mac_File::Mac_File( short r ) : Mac_File_Reader( r ) {
}

bool FileExists( const std::wstring& strPath )
{
    assert( strPath.length() );
    
    char szMbPath[PATH_MAX];
    wcstombs( szMbPath, strPath.data(), strPath.length() );
    return ( access( szMbPath, F_OK ) != -1 );
}

bool AreFilesEqual( const std::wstring& strPath1, const std::wstring& strPath2 )
{
    assert( 0 );
    
    char szTemp[PATH_MAX], szTemp2[PATH_MAX], szTemp3[PATH_MAX];
    wcstombs( szTemp, strPath1.data(), strPath1.length() );
    char* pszError = realpath( szTemp2, szTemp );
    assert( pszError );
    wcstombs( szTemp, strPath2.data(), strPath2.length() );
    pszError = realpath( szTemp3, szTemp );
    assert( pszError );
    
    return (strcmp( szTemp2, szTemp3 ) == 0);
}

void CreateAlias( const GaMBLFileHandle& original, std::wstring& strLinkName )
{
    assert(0);
}

GaMBLFileHandle create_file( const GaMBLFileHandle& dir, const std::wstring& name )
{
    std::wstring strNewFilename;
    dir.GetFilePath( strNewFilename, true );
    strNewFilename += name;
    
	return GaMBLFileHandle( strNewFilename, "W+" );;
}

void sanitize_filename( std::wstring& str )
{
	for ( int i = str.length(); i--; )
	{
		wchar_t& c = str[i];
		if ( c == L':' )
			c = L'-';
		else if ( 128 <= c && c <= 255 )
			c = L' ';
	}
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