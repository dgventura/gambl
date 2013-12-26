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

// Read-only file
class Mac_File_Reader : public File_Reader
{
public:
	Mac_File_Reader( const GaMBLFileHandle&, short perm = fsRdPerm );
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

bool AreFilesEqual( const std::wstring& strPath1, const std::wstring& strPath2 );
bool has_extension( const char* str, const char* suffix );


#endif
