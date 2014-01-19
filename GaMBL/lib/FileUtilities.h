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
	
	void read( const GaMBLFileHandle& FileHandle )
    {
        assert( FileHandle.IsOk() );
        
        ref_ = FileHandle;
        int nFail = fstat( ref_.GetDescriptor(), &m_FileInfo );
        if ( nFail )
        {
            printf( "File info error: %s\n", strerror(errno) );
            assert( 0 );
        }
        memset( &m_LegacyInfo, 2, sizeof(m_LegacyInfo) );
        
        std::wstring strTemp;
        FileHandle.GetFilePath( strTemp, true );
        std::wcout << "Reading Cat Info for " << strTemp << std::endl;
    }
	//void write( const GaMBLFileHandle&, FSCatalogInfoBitmap );
	
	bool is_dir() const
    {
        return S_ISDIR(m_FileInfo.st_mode);
    }
	GMBFileInfo& finfo()       {
        return m_LegacyInfo;
    }
	const GMBFileInfo& finfo() const
    {
        return m_LegacyInfo;
    }
	bool is_alias() const
    {
        assert( m_FileInfo.st_mode );
        return S_ISLNK(m_FileInfo.st_mode);
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
//bool remove_filename_extension( HFSUniStr255& );
bool remove_filename_extension( char* );
// Copy file's name without extension to 'out' (if directory, just copy name unchanged).
void filename_without_extension( const std::wstring&, char* out );
// Get parent directory
std::wstring get_parent( const std::wstring& );
std::wstring StripPath( const std::wstring& strFullPath );

void str_to_filename( const char*, std::wstring& );
void filename_to_str( const std::wstring&, char* );

// Replace/remove illegal characters (i.e. ':')
void sanitize_filename( std::wstring& strFilename );

void CreateAlias( const GaMBLFileHandle& original, std::wstring& strLinkName );
bool FileExists( const std::wstring& strFilename );

// Create file of given type and creator. Throw exception if file already exists.
GaMBLFileHandle create_file( const std::wstring& dir, const std::wstring& name );

std::wstring GetBundlePath();

#endif
