
// Game_Music_Box 0.5.2. http://www.slack.net/~ant/game-music-box

#include "file_util.h"

#include <cctype>
//#include <Folders.h>
#include "mac_util.h"

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

// Cat_Info

bool Cat_Info::read( FSIterator iter, FSCatalogInfoBitmap flags, HFSUniStr255* name )
{
	return FSGetCatalogInfoBulkChk( iter, 1, NULL, flags, this, &ref_, NULL, name );
}

void Cat_Info::read( const FSRef& ref, FSCatalogInfoBitmap flags, HFSUniStr255* name,
		FSSpec* spec, FSRef* parent )
{
	ref_ = ref;
	FSGetCatalogInfoChk( ref, flags, this, name, spec, parent );
}

void Cat_Info::write( const FSRef& ref, FSCatalogInfoBitmap flags )
{
	throw_file_error( FSSetCatalogInfo( &ref, flags, this ), ref );
}

// Dir_Iterator

FS_Iterator::FS_Iterator( const FSRef& ref, FSIteratorFlags flags )
{
	throw_file_error( FSOpenIterator( &ref, flags, &iter ), ref );
}

FS_Iterator::~FS_Iterator()
{
	debug_if_error( FSCloseIterator( iter ) );
}

Dir_Iterator::Dir_Iterator( const FSRef& dir, bool sh ) :
	iter( dir ),
	skip_hidden( sh )
{
}

bool Dir_Iterator::next( FSCatalogInfoBitmap flags, HFSUniStr255* name )
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

void FSGetCatalogInfoChk( const FSRef& ref, FSCatalogInfoBitmap flags, FSCatalogInfo* info,
		HFSUniStr255* name, FSSpec* spec, FSRef* parent )
{
	throw_file_error( FSGetCatalogInfo( &ref, flags, info, name, spec, parent ), ref );
}

int FSGetCatalogInfoBulkChk( FSIterator iter, ItemCount count, Boolean* changed,
		FSCatalogInfoBitmap flags, FSCatalogInfo* info, FSRef* fsref, FSSpec* spec,
		HFSUniStr255* name )
{
	ItemCount result = 0;
	OSErr err = FSGetCatalogInfoBulk( iter, count, &result, changed, flags, info, fsref,
			spec, name );
	if ( err == errFSNoMoreItems )
		return 0;
	throw_if_error( err );
	assert( result > 0 );
	return result;
}

void FSCreateDirectoryUnicodeChk( const FSRef& dir, const HFSUniStr255& name,
		FSCatalogInfoBitmap flags, const FSCatalogInfo* info, FSRef* fsref,
		FSSpec* fsspec, UInt32* id )
{
	throw_file_error( FSCreateDirectoryUnicode( &dir, name.length, name.unicode, flags,
			info, fsref, fsspec, id ), dir, name );
}

FSRef get_parent( const FSRef& fsref )
{
	FSRef result;
	FSGetCatalogInfoChk( fsref, 0, NULL, NULL, NULL, &result );
	return result;
}

FSRef get_bundle_fsref()
{
	FSRef fsref;
	static const ProcessSerialNumber current = { 0, kCurrentProcess };
	throw_if_error( GetProcessBundleLocation( &current, &fsref ) );
	return fsref;
}

bool is_dir( const FSRef& path )
{
	Cat_Info info;
	info.read( path, kFSCatInfoNodeFlags );
	return info.is_dir();
}

FSRef FindFolderChk( OSType type, int volume )
{
	FSRef result;
	throw_if_error( FSFindFolder( volume, type, true, &result ) );
	return result;
}

// File_Swapper

File_Swapper::File_Swapper( const FSRef& in ) : orig_path( in )
{
	char name [32];
	num_to_str( TickCount(), name );
	HFSUniStr255 filename;
	str_to_filename( name, filename );
	
	Cat_Info info;
	info.read( orig_path, kFSCatInfoVolume );
	FSRef temp_dir;
	throw_file_error( FSFindFolder( info.volume, kTemporaryFolderType, true, &temp_dir ), in );
	
	temp_path = create_file( temp_dir, filename, '\?\?\?\?', '\?\?\?\?' );
	swapped = false;
}

File_Swapper::~File_Swapper()
{
	debug_if_error( FSDeleteObject( &temp_path ) );
}
	
void File_Swapper::swap()
{
	throw_file_error( FSExchangeObjects( &temp_path, &orig_path ), orig_path );
	swapped = true;
}

// File_Deleter

File_Deleter::File_Deleter( const FSRef& r ) : ref( r ), valid( true )
{
}

File_Deleter::~File_Deleter()
{
	if ( valid )
		debug_if_error( FSDeleteObject( &ref ) );
}

void File_Deleter::set( const FSRef& r )
{
	ref = r;
	valid = true;
}

void File_Deleter::clear()
{
	valid = false;
}

// Mac_File

Mac_File_Reader::Mac_File_Reader( const FSRef& file, short perm ) :
	fsref( file ),
	fsref_valid( true )
{
	unclosed = true;
	mode = 0;
	throw_error( FSOpenFork( &file, 0, NULL, perm, &ref ) );
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

long Mac_File_Reader::size() const
{
	SInt64 size;
	throw_error( FSGetForkSize( ref, &size ) );
	return size;
}

void Mac_File::set_size( long s )
{
	throw_error( FSSetForkSize( ref, fsFromStart, s ) );
}

long Mac_File_Reader::tell() const
{
	SInt64 pos;
	throw_error( FSGetForkPosition( ref, &pos ) );
	return pos;
}

Mac_File_Reader::error_t Mac_File_Reader::seek( long n )
{
	throw_error( FSSetForkPosition( ref, fsFromStart, n ) );
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
	throw_unless( FSReadFork( ref, fsAtMark + mode, 0, s, p, &count ), eofErr );
	return count;
}

void Mac_File_Reader::close()
{
	if ( unclosed ) {
		unclosed = false;
		debug_if_error( FSCloseFork( ref ) );
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
	throw_error( FSWriteFork( ref, fsAtMark + mode, 0, s, p, &count ) );
	if ( count != s )
		throw_error( ioErr );
	return NULL;
}

// misc

FSRef create_file( const FSRef& dir, const HFSUniStr255& name,
		OSType type, OSType creator, int flags )
{
	FSRef ref;
	FSCatalogInfo cat_info;
	FileInfo& info = *reinterpret_cast<FileInfo*> (cat_info.finderInfo);
	info.fileType = type;
	info.fileCreator = creator;
	info.finderFlags = flags;
	info.location.h = 0;
	info.location.v = 0;
	info.reservedField = 0;
	throw_file_error( FSCreateFileUnicode( &dir, name.length, name.unicode,
			kFSCatInfoFinderInfo, &cat_info, &ref, NULL ), dir, name );
	return ref;
}

bool FSMakeFSRefExists( const FSRef& dir, const HFSUniStr255& name, FSRef* ref )
{
	FSRef ref2;
	if ( !ref )
		ref = &ref2;
	OSErr err = FSMakeFSRefUnicode( &dir, name.length, name.unicode, 0, ref );
	if ( err == fnfErr )
		return false;
	throw_file_error( err, dir, name );
	return true;
}

FSRef FSMakeFSRefChk( const FSRef& dir, const HFSUniStr255& name )
{
	FSRef ref;
	FSMakeFSRefExists( dir, name, &ref );
	return ref;
}

bool exists( const FSRef& fsref )
{
	FSRef dir;
	HFSUniStr255 name;
	return noErr == FSGetCatalogInfo( &fsref, 0, NULL, &name, NULL, &dir );
}

bool FSResolveAliasFileExists( const FSRef& in, FSRef* out )
{
	FSRef dummy;
	if ( !out )
		out = &dummy;
	*out = in;
	Boolean is_dir, is_alias;
	OSErr err = FSResolveAliasFile( out, true, &is_dir, &is_alias );
	return !err;
}

FSRef FSResolveAliasFileChk( const FSRef& in )
{
	FSRef result = in;
	Boolean is_dir, is_alias;
	OSErr err = FSResolveAliasFile( &result, true, &is_dir, &is_alias );
	
	// File not found and directory not found errors are apparently swapped
	if ( err == fnfErr )
		err = dirNFErr;
	else if ( err == dirNFErr )
		err = fnfErr;
	
	throw_file_error( err, in );
	return result;
}

// filenames

void FSRenameUnicodeChk( const FSRef& path, const HFSUniStr255& name, FSRef* new_ref )
{
	throw_file_error( FSRenameUnicode( &path, name.length, name.unicode, 0, new_ref ), path );
}

void get_filename( const FSRef& path, char* out, long size )
{
	Cat_Info info;
	HFSUniStr255 filename;
	info.read( path, kFSCatInfoNodeFlags, &filename );
	if ( filename.length >= size )
		filename.length = size - 1; // to do: provide way to avoid truncation
	filename_to_str( filename, out );
}

void filename_without_extension( const FSRef& fsref, char* out )
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

void make_alias_file( const FSRef& original, const FSRef& dir, const HFSUniStr255& name )
{
	Cat_Info info;
	info.read( original, kFSCatInfoFinderInfo );
	
	info.finfo().finderFlags = kIsAlias;
	info.finfo().location.h = 0;
	info.finfo().location.v = 0;
	info.finfo().reservedField = 0;
	
	FSRef alias;
	FSCreateResFile( &dir, name.length, name.unicode,
			kFSCatInfoFinderInfo, &info, &alias, NULL );
	throw_file_error( ResError(), dir, name );
	File_Deleter deleter( alias );
	
	AliasHandle alias_data = NULL;
	throw_if_error( FSNewAlias( NULL, &original, &alias_data ) );
	require( alias_data );
	
	short ref = FSOpenResFile( &alias, fsRdWrPerm );
	throw_file_error( ResError(), alias );
	// to do: don't leak alias if open fails
	
	// to do: handle failed add
	AddResource( (Handle) alias_data, 'alis', 0, "\p" );
	CloseResFile( ref );
	deleter.clear();
}

