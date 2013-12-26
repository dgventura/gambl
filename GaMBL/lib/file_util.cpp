
// Game_Music_Box 0.5.2. http://www.slack.net/~ant/game-music-box

#include "FileUtilities.h"

#include <cctype>

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
#if 0
// Cat_Info

bool Cat_Info::read( DeprecatedFSIterator iter, DeprecatedFSCatalogInfoBitmap flags, HFSUniStr255* name )
{
	return FSGetCatalogInfoBulk( iter, 1, NULL, flags, this, &ref_, NULL, name );
}

void Cat_Info::read( const GaMBLFileHandle& ref, DeprecatedFSCatalogInfoBitmap flags, HFSUniStr255* name,
		FSSpec* spec, GaMBLFileHandle* parent )
{
	ref_ = ref;
	FSGetCatalogInfoChk( ref, flags, this, name, spec, parent );
}

void Cat_Info::write( const GaMBLFileHandle& ref, DeprecatedFSCatalogInfoBitmap flags )
{
	throw_file_error( DeprecatedFSSetCatalogInfo( &ref, flags, this ), ref );
}

// Dir_Iterator

FS_Iterator::FS_Iterator( const GaMBLFileHandle& ref, DeprecatedFSIteratorFlags flags )
{
	throw_file_error( DeprecatedFSOpenIterator( &ref, flags, &iter ), ref );
}

FS_Iterator::~FS_Iterator()
{
	debug_if_error( DeprecatedFSCloseIterator( iter ) );
}

Dir_Iterator::Dir_Iterator( const GaMBLFileHandle& dir, bool sh ) :
	iter( dir ),
	skip_hidden( sh )
{
}

bool Dir_Iterator::next( DeprecatedFSCatalogInfoBitmap flags, HFSUniStr255* name )
{
	while ( Cat_Info::read( iter, flags | kFSCatInfoNodeFlags | kFSCatInfoFinderInfo, name ) )
	{
		if ( !skip_hidden || !is_hidden() )
			return true;
	}
	
	return false;
}

// directories

bool is_dir_type( OSType type )
{
	switch ( type )
	{
		case 'fold':
		case 'fdrp':
		case 'disk':
			return true;
	}
	
	return false;
}

void DeprecatedFSGetCatalogInfoChk( const GaMBLFileHandle& ref, DeprecatedFSCatalogInfoBitmap flags, DeprecatedFSCatalogInfo* info,
		HFSUniStr255* name, DeprecatedFSSpec* spec, GaMBLFileHandle* parent )
{
	throw_file_error( DeprecatedFSGetCatalogInfo( &ref, flags, info, name, spec, parent ), ref );
}

int DeprecatedFSGetCatalogInfoBulkChk( DeprecatedFSIterator iter, ItemCount count, Boolean* changed,
		FSCatalogInfoBitmap flags, DeprecatedFSCatalogInfo* info, GaMBLFileHandle* fsref, DeprecatedFSSpec* spec,
		HFSUniStr255* name )
{
	ItemCount result = 0;
	OSErr err = DeprecatedFSGetCatalogInfoBulk( iter, count, &result, changed, flags, info, fsref,
			spec, name );
	if ( err == errFSNoMoreItems )
		return 0;
	throw_if_error( err );
	assert( result > 0 );
	return result;
}

void DeprecatedFSCreateDirectoryUnicodeChk( const GaMBLFileHandle& dir, const HFSUniStr255& name,
		FSCatalogInfoBitmap flags, const DeprecatedFSCatalogInfo* info, GaMBLFileHandle* fsref,
		FSSpec* fsspec, UInt32* id )
{
	throw_file_error( DeprecatedFSCreateDirectoryUnicode( &dir, name.length, name.unicode, flags,
			info, fsref, fsspec, id ), dir, name );
}

GaMBLFileHandle get_parent( const GaMBLFileHandle& fsref )
{
	GaMBLFileHandle result;
	FSGetCatalogInfoChk( fsref, 0, NULL, NULL, NULL, &result );
	return result;
}

GaMBLFileHandle get_bundle_fsref()
{
	GaMBLFileHandle fsref;
	static const ProcessSerialNumber current = { 0, kCurrentProcess };
	throw_if_error( GetProcessBundleLocation( &current, &fsref ) );
	return fsref;
}

bool is_dir( const GaMBLFileHandle& path )
{
	Cat_Info info;
	info.read( path, kFSCatInfoNodeFlags );
	return info.is_dir();
}

GaMBLFileHandle FindFolderChk( OSType type, int volume )
{
	GaMBLFileHandle result;
	throw_if_error( DeprecatedFSFindFolder( volume, type, true, &result ) );
	return result;
}

// File_Swapper

File_Swapper::File_Swapper( const GaMBLFileHandle& in ) : orig_path( in )
{
	char name [32];
	num_to_str( TickCount(), name );
	HFSUniStr255 filename;
	str_to_filename( name, filename );
	
	Cat_Info info;
	info.read( orig_path, kFSCatInfoVolume );
	GaMBLFileHandle temp_dir;
	throw_file_error( DeprecatedFSFindFolder( info.volume, kTemporaryFolderType, true, &temp_dir ), in );
	
	temp_path = create_file( temp_dir, filename, '\?\?\?\?', '\?\?\?\?' );
	swapped = false;
}

File_Swapper::~File_Swapper()
{
	debug_if_error( DeprecatedFSDeleteObject( &temp_path ) );
}
	
void File_Swapper::swap()
{
	throw_file_error( DeprecatedFSExchangeObjects( &temp_path, &orig_path ), orig_path );
	swapped = true;
}

// File_Deleter

File_Deleter::File_Deleter( const GaMBLFileHandle& r ) : ref( r ), valid( true )
{
}

File_Deleter::~File_Deleter()
{
	if ( valid )
		debug_if_error( DeprecatedFSDeleteObject( &ref ) );
}

void File_Deleter::set( const GaMBLFileHandle& r )
{
	ref = r;
	valid = true;
}

void File_Deleter::clear()
{
	valid = false;
}

// Mac_File

Mac_File_Reader::Mac_File_Reader( const GaMBLFileHandle& file, short perm ) :
	fsref( file ),
	fsref_valid( true )
{
	unclosed = true;
	mode = 0;
    OSStatus status = DeprecatedFSOpenFork( &file, 0, NULL, perm, &ref );
    bool bValidFSRef = true;
    if ( status != noErr )
        bValidFSRef = DeprecatedFSIsFSRefValid( &file );
    assert ( bValidFSRef );
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
	SInt64 size;
	throw_error( DeprecatedFSGetForkSize( ref, &size ) );
	return size;
}

void Mac_File::set_size( long s )
{
	throw_error( DeprecatedFSSetForkSize( ref, fsFromStart, s ) );
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
		debug_if_error( DeprecatedFSCloseFork( ref ) );
	}
	ref = -1;
}

void Mac_File_Reader::throw_error( long err )
{
	if ( !err )
		return;
	if ( !fsref_valid && !FSGetForkCBInfo( ref, 0, NULL, NULL, NULL, &fsref, NULL ) )
		fsref_valid = true;
	if ( fsref_valid )
		throw_file_error( err, fsref );
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
Mac_File::Mac_File( const GaMBLFileHandle& r ) : Mac_File_Reader( r, fsRdWrPerm ) {
}
Mac_File::Mac_File( short r ) : Mac_File_Reader( r ) {
}

// misc

GaMBLFileHandle create_file( const GaMBLFileHandle& dir, const HFSUniStr255& name,
		OSType type, OSType creator, int flags )
{
	GaMBLFileHandle ref;
	FSCatalogInfo cat_info;
	FileInfo& info = *reinterpret_cast<FileInfo*> (cat_info.finderInfo);
	info.fileType = type;
	info.fileCreator = creator;
	info.finderFlags = flags;
	info.location.h = 0;
	info.location.v = 0;
	info.reservedField = 0;
	throw_file_error( DeprecatedFSCreateFileUnicode( &dir, name.length, name.unicode,
			kFSCatInfoFinderInfo, &cat_info, &ref, NULL ), dir, name );
	return ref;
}

bool DeprecatedFSMakeFSRefExists( const GaMBLFileHandle& dir, const HFSUniStr255& name, GaMBLFileHandle* ref )
{
	GaMBLFileHandle ref2;
	if ( !ref )
		ref = &ref2;
	OSErr err = DeprecatedFSMakeFSRefUnicode( &dir, name.length, name.unicode, 0, ref );
	if ( err == fnfErr )
		return false;
	throw_file_error( err, dir, name );
	return true;
}

GaMBLFileHandle DeprecatedFSMakeFSRefChk( const GaMBLFileHandle& dir, const HFSUniStr255& name )
{
	GaMBLFileHandle ref;
	FSMakeFSRefExists( dir, name, &ref );
	return ref;
}

bool exists( const GaMBLFileHandle& fsref )
{
	GaMBLFileHandle dir;
	HFSUniStr255 name;
	return noErr == DeprecatedFSGetCatalogInfo( &fsref, 0, NULL, &name, NULL, &dir );
}

bool DeprecatedFSResolveAliasFileExists( const GaMBLFileHandle& in, GaMBLFileHandle* out )
{
	GaMBLFileHandle dummy;
	if ( !out )
		out = &dummy;
	*out = in;
	Boolean is_dir, is_alias;
	OSErr err = DeprecatedFSResolveAliasFile( out, true, &is_dir, &is_alias );
	return !err;
}

GaMBLFileHandle DeprecatedFSResolveAliasFileChk( const GaMBLFileHandle& in )
{
	GaMBLFileHandle result = in;
	Boolean is_dir, is_alias;
	OSErr err = DeprecatedFSResolveAliasFile( &result, true, &is_dir, &is_alias );
	
	// File not found and directory not found errors are apparently swapped
	if ( err == fnfErr )
		err = dirNFErr;
	else if ( err == dirNFErr )
		err = fnfErr;
	
	throw_file_error( err, in );
	return result;
}

// filenames

void DeprecatedFSRenameUnicodeChk( const GaMBLFileHandle& path, const HFSUniStr255& name, GaMBLFileHandle* new_ref )
{
	throw_file_error( DeprecatedFSRenameUnicode( &path, name.length, name.unicode, 0, new_ref ), path );
}

void get_filename( const GaMBLFileHandle& path, char* out, long size )
{
	Cat_Info info;
	HFSUniStr255 filename;
	info.read( path, kFSCatInfoNodeFlags, &filename );
	if ( filename.length >= size )
		filename.length = size - 1; // to do: provide way to avoid truncation
	filename_to_str( filename, out );
}

void filename_without_extension( const GaMBLFileHandle& fsref, char* out )
{
	Cat_Info info;
	HFSUniStr255 filename;
	info.read( fsref, kFSCatInfoNodeFlags, &filename );
	if ( !info.is_dir() )
		remove_filename_extension( filename );
	filename_to_str( filename, out );
}

void sanitize_filename( HFSUniStr255& str )
{
	for ( int i = str.length; i--; )
	{
		UniChar& c = str.unicode [i];
		if ( c == ':' )
			c = '-';
		else if ( 128 <= c && c <= 255 )
			c = ' ';
	}
}


bool has_extension( const HFSUniStr255& s, const char* ext )
{
	int elen = std::strlen( ext );
	int len = s.length;
	if ( len < elen )
		return false;
	for ( int i = 0; i < elen; i++ )
		if ( std::tolower( s.unicode [len - elen + i] ) != ext [i] )
			return false;
	return true;
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

void str_to_filename( const char* in, HFSUniStr255& out )
{
	int i = 0;
	while ( i < 255 && in [i] ) {
		out.unicode [i] = (unsigned char) in [i];
		i++;
	}
	out.length = i;
}

void filename_to_str( const HFSUniStr255& in, char* out )
{
	int i = in.length;
	out [i] = 0;
	while ( i-- )
		out [i] = in.unicode [i];
}

const char* get_filename_extension( const char* str )
{
	// to do: proper unicode truncation (see Apple TN2078)
	// to do: better filename extension determination (might not be 3/4 chars)
	
	int len = std::strlen( str );
	
	if ( len > 4 && str [len - 4] == '.' )
		return &str [len - 4];
	
	if ( len > 5 && str [len - 5] == '.' )
		return &str [len - 5];
	
	return NULL;
}

bool remove_filename_extension( char* str )
{
	const char* pos = get_filename_extension( str );
	if ( !pos )
		return false;
	
	*const_cast<char*> (pos) = 0;
	return true;
}

bool remove_filename_extension( HFSUniStr255& str )
{
	// to do: proper unicode truncation (see Apple TN2078)
	// to do: better filename extension determination (might not be 3/4 chars)
	
	int len = str.length;
	
	if ( len > 4 && str.unicode [len - 4] == '.' ) {
		str.length = len - 4;
		return true;
	}
	
	if ( len > 5 && str.unicode [len - 5] == '.' ) {
		str.length = len - 5;
		return true;
	}
	
	return false;
}

void make_alias_file( const GaMBLFileHandle& original, const GaMBLFileHandle& dir, const HFSUniStr255& name )
{
	Cat_Info info;
	info.read( original, kFSCatInfoFinderInfo );
	
	info.finfo().finderFlags = kIsAlias;
	info.finfo().location.h = 0;
	info.finfo().location.v = 0;
	info.finfo().reservedField = 0;
	
	GaMBLFileHandle alias;
	FSCreateResFile( &dir, name.length, name.unicode,
			kFSCatInfoFinderInfo, &info, &alias, NULL );
	throw_file_error( ResError(), dir, name );
	File_Deleter deleter( alias );
	
	AliasHandle alias_data = NULL;
	throw_if_error( DeprecatedFSNewAlias( NULL, &original, &alias_data ) );
	require( alias_data );
	
	short ref = DeprecatedFSOpenResFile( &alias, fsRdWrPerm );
	throw_file_error( ResError(), alias );
	// to do: don't leak alias if open fails
	
	// to do: handle failed add
	AddResource( (Handle) alias_data, 'alis', 0, "\p" );
	CloseResFile( ref );
	deleter.clear();
}

#endif