//
//  FileUtilities.h
//  GaMBL
//
//  Created by David Ventura on 12/25/13.
//  Copyright (c) 2013 David Ventura. All rights reserved.
//

#ifndef GaMBL_FileUtilities_h
#define GaMBL_FileUtilities_h

#include "common.h"
#include "abstract_file.h"
#include <dirent.h>
#include <sys/stat.h>

struct GMBFileInfo {
    OSType fileType;
    OSType fileCreator;
    UInt16 finderFlags;
    Point location;
    UInt16 reservedField;
};

// Catalog info
class Cat_Info
{
	GaMBLFileHandle ref_;
    struct stat m_FileInfo;
    GMBFileInfo m_LegacyInfo;
    
public:
	const GaMBLFileHandle& ref() const { return ref_; }
	
	//bool read( FSIterator, FSCatalogInfoBitmap, HFSUniStr255* name = NULL );
	void read( const GaMBLFileHandle& FileHandle );
	//void write( const GaMBLFileHandle&, FSCatalogInfoBitmap );
	
	bool is_dir() const
    {
        return m_FileInfo.st_mode & S_IFDIR;
    }
	GMBFileInfo& finfo()       { return m_LegacyInfo; }
	const GMBFileInfo& finfo() const { return m_LegacyInfo; }
	bool is_hidden() const
    {
        //TODO RAD
        return false;
//        return m_FileInfo.st_mode & S_IF
    }
	bool is_alias() const
    {
        assert( m_FileInfo.st_mode );
        return m_FileInfo.st_mode & S_IFLNK;
    }
    long size() const
    {
        assert( m_FileInfo.st_size );
        return m_FileInfo.st_size;
    }
};

class Mac_File;

// Read-only file
class Mac_File_Reader : public File_Reader
{
public:
	Mac_File_Reader( const std::wstring&, short perm = fsRdPerm );
	Mac_File_Reader( short ref_num );
	~Mac_File_Reader();
	short ref_num() const { return ref; }
	
	// If false, access file using uncached mode
    void set_cached( bool b );
	
	long size();
	
	// Reports errors with exceptions; error_t is always NULL.
	long tell();
	error_t seek( long );
	
	error_t read( void*, long );
	long read_avail( void*, long );
	
	void close();
	
private:
	GaMBLFileHandle fsref;
	bool fsref_valid;
	FSIORefNum ref;
	short mode;
	bool unclosed;
	
	void throw_error( long );
	friend class Mac_File;
};

// Read/write file
class Mac_File : public Mac_File_Reader, public Data_Writer {
public:
	Mac_File( const std::wstring& );
	Mac_File( short ref_num );
	
	void set_size( long );
	
	// Reports errors with exceptions; error_t is always NULL.
	Data_Writer::error_t write( const void*, long );
};

bool AreFilesEqual( const std::wstring& strPath1, const std::wstring& strPath2 );
bool has_extension( const char* str, const char* suffix );
// Remove extension from filename. True if string was modified.
bool remove_filename_extension( HFSUniStr255& );
bool remove_filename_extension( char* );
// Copy file's name without extension to 'out' (if directory, just copy name unchanged).
void filename_without_extension( const std::wstring&, char* out );
// Get parent directory
std::wstring get_parent( const std::wstring& );
void str_to_filename( const char*, HFSUniStr255& );
void filename_to_str( const HFSUniStr255&, char* );

// Create file of given type and creator. Throw exception if file already exists.
GaMBLFileHandle create_file( const std::wstring& dir, const std::wstring& name,
                            OSType type = 0, OSType creator = 0, int flags = 0 );

#endif
