
// Common interface to ZIP and RAR archives or a single file

// Game_Music_Box 0.5.2. Copyright (C) 2005 Shay Green. GNU LGPL license.

#ifndef FILE_ARCHIVE_H
#define FILE_ARCHIVE_H

#include "common.h"
#include "abstract_file.h"

class File_Archive {
public:
	File_Archive();
	virtual ~File_Archive() { }
	
	// Jump to a given file within the archive. False if index is beyond end.
	virtual bool seek( int index, bool might_extract = false ) = 0;
	
	// Info for current item
	struct info_t {
		bool is_file;
		const char* name;
		long size;
	};
	const info_t& info() const;
	
	// Extract current file. Caller must delete reader when done with it.
	virtual Data_Reader* extract();
	
	// Extract first 'n' bytes of current file into buffer. More efficient
	// when archive reader must use a temporary buffer for the normal extract().
	virtual void extract( void*, long n );
	
	// Close file and free memory for archive. Next seek() will re-open archive.
	virtual void uncache();
	
protected:
	info_t info_;
};

// Treat a single file as an archive with a single file named 'filename'
File_Archive* open_file_archive( const std::wstring& strPath, const char* filename );

// Open ZIP archive
File_Archive* open_zip_archive( const std::wstring& strPath );

// Open RAR archive
File_Archive* open_rar_archive( const std::wstring& strPath );

inline const File_Archive::info_t& File_Archive::info() const {
	return info_;
}

#endif

