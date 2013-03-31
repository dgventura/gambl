
// Mac file utilities

// Game_Music_Box 0.5.2. Copyright (C) 2005 Shay Green. GNU LGPL license.

#ifndef FILE_UTIL_H
#define FILE_UTIL_H

#include "common.h"
#include "abstract_file.h"
//#include <Files.h>
//#include <Folders.h>

// Maximum length of Mac filename (only on HFS/HFS+ volumes?)
const int max_filename = 31;

// Delete file on destruction unless told not to
class File_Deleter : noncopyable {
public:
	File_Deleter() : valid( false ) { }
	File_Deleter( const FSRef& );
	~File_Deleter();
	void set( const FSRef& );
	void clear();
private:
	FSRef ref;
	bool valid;
};

// Catalog info
class Cat_Info : public FSCatalogInfo {
	FSRef ref_;
public:
	const FSRef& ref() const { return ref_; }
	
	bool read( FSIterator, FSCatalogInfoBitmap, HFSUniStr255* name = NULL );
	void read( const FSRef&, FSCatalogInfoBitmap, HFSUniStr255* name = NULL,
			FSSpec* = NULL, FSRef* parent = NULL );
	void write( const FSRef&, FSCatalogInfoBitmap );
	
	bool is_dir() const     { return nodeFlags & kFSNodeIsDirectoryMask; }
	FileInfo& finfo()       { return *reinterpret_cast<FileInfo*> (finderInfo); }
	const FileInfo& finfo() const { return *reinterpret_cast<const FileInfo*> (finderInfo); }
	bool is_hidden() const  { return finfo().finderFlags & kIsInvisible; }
	bool is_alias() const   { return finfo().finderFlags & kIsAlias; }
};

// Directory iterator holder
class FS_Iterator : noncopyable {
	FSIterator iter;
public:
	explicit FS_Iterator( const FSRef&, FSIteratorFlags = kFSIterateFlat );
	~FS_Iterator();
	
	operator FSIterator () const { return iter; }
};

// Directory iterator
class Dir_Iterator : public Cat_Info {
	FS_Iterator iter;
	bool skip_hidden;
public:
	explicit Dir_Iterator( const FSRef&, bool skip_hidden = true );
	
	// default flags are kFSCatInfoNodeFlags | kFSCatInfoFinderInfo
	bool next( FSCatalogInfoBitmap extra_flags = 0, HFSUniStr255* name = NULL );
};

class File_Swapper {
public:
	// Create temporary file on same volume as original
	File_Swapper( const FSRef& original );
	
	// Delete temporary file
	~File_Swapper();
	
	// Path of temporary file
	operator const FSRef& () const { return temp_path; }
	
	// Swap contents of temporary file with that of original file
	void swap();
	
private:
	FSRef temp_path;
	FSRef orig_path;
	bool swapped;
};
class Mac_File;

// Read-only file
class Mac_File_Reader : public File_Reader {
public:
	Mac_File_Reader( const FSRef&, short perm = fsRdPerm );
	Mac_File_Reader( short ref_num );
	~Mac_File_Reader();
	short ref_num() const { return ref; }
	
	// If false, access file using uncached mode
	void set_cached( bool );
	
	long size() const;
	
	// Reports errors with exceptions; error_t is always NULL.
	long tell() const;
	error_t seek( long );
	
	error_t read( void*, long );
	long read_avail( void*, long );
	
	void close();
	
private:
	FSRef fsref;
	bool fsref_valid;
	short ref;
	short mode;
	bool unclosed;
	
	void throw_error( long );
	friend class Mac_File;
};

// Read/write file
class Mac_File : public Mac_File_Reader, public Data_Writer {
public:
	Mac_File( const FSRef& );
	Mac_File( short ref_num );
	
	void set_size( long );
	
	// Reports errors with exceptions; error_t is always NULL.
	Data_Writer::error_t write( const void*, long );
};

// Create file of given type and creator. Throw exception if file already exists.
FSRef create_file( const FSRef& dir, const HFSUniStr255& name,
		OSType type = 0, OSType creator = 0, int flags = 0 );

// True if given dir:name exists. Optionally create FSRef to item.
bool FSMakeFSRefExists( const FSRef& dir, const HFSUniStr255& name, FSRef* out_ref = NULL );

// Get parent directory
FSRef get_parent( const FSRef& );

// True if file/directory exists
bool exists( const FSRef& );

void get_filename( const FSRef&, char* out, long size );

// Copy file's name without extension to 'out' (if directory, just copy name unchanged).
void filename_without_extension( const FSRef&, char* out );

// Pointer to extension of filename (including '.'), or NULL if it has none.
const char* get_filename_extension( const char* );

// Remove extension from filename. True if string was modified.
bool remove_filename_extension( HFSUniStr255& );
bool remove_filename_extension( char* );

// True if 'filename' ends with 'ext'. Ext must be lowercase extension, i.e. ".txt".
bool has_extension( const HFSUniStr255& filename, const char* ext );

// True if 'filename' ends with 'ext'. Ext must be uppercase extension, i.e. ".TXT".
bool has_extension( const char*, const char* ext );

// FSRef of application bundle
FSRef get_bundle_fsref();

void str_to_filename( const char*, HFSUniStr255& );
void filename_to_str( const HFSUniStr255&, char* );

bool is_dir( const FSRef& );

// True if type corresponds to directory (fdrp, disk, fold, etc.)
bool is_dir_type( OSType );

bool FSResolveAliasFileExists( const FSRef&, FSRef* out = NULL );

// Replace/remove illegal characters (i.e. ':')
void sanitize_filename( HFSUniStr255& );

void make_alias_file( const FSRef& original, const FSRef& dir, const HFSUniStr255& name );

FSRef FindFolderChk( OSType, int volume = kOnAppropriateDisk );

// FunctionChk basically wraps the filesystem call and throws an exception instead of
// returning an error code.

void FSRenameUnicodeChk( const FSRef&, const HFSUniStr255& name, FSRef* new_ref = NULL );

int FSGetCatalogInfoBulkChk( FSIterator, ItemCount, Boolean* changed, FSCatalogInfoBitmap,
		FSCatalogInfo*, FSRef* = NULL, FSSpec* = NULL, HFSUniStr255* = NULL );

void FSGetCatalogInfoChk( const FSRef&, FSCatalogInfoBitmap, FSCatalogInfo*,
		HFSUniStr255* = NULL, FSSpec* = NULL, FSRef* parent = NULL );

void FSCreateDirectoryUnicodeChk( const FSRef& dir, const HFSUniStr255& name,
		FSCatalogInfoBitmap, const FSCatalogInfo* = NULL, FSRef* = NULL,
		FSSpec* = NULL, UInt32* id = NULL );

FSRef FSResolveAliasFileChk( const FSRef& );

FSRef FSMakeFSRefChk( const FSRef& dir, const HFSUniStr255& name );


// End of public interface
inline void Mac_File_Reader::set_cached( bool b ) {
	mode = (b ? 0 : noCacheMask);
}
inline Mac_File::Mac_File( const FSRef& r ) : Mac_File_Reader( r, fsRdWrPerm ) {
}
inline Mac_File::Mac_File( short r ) : Mac_File_Reader( r ) {
}
#endif

