
// Extracts files from zip archives using abstract file interface

// Zip_Extractor 0.1.0. Copyright (C) 2005 Shay Green. GNU LGPL license.

#ifndef ZIP_EXTRACTOR_H
#define ZIP_EXTRACTOR_H

#include "abstract_file.h"

class Zip_Extractor {
public:
	Zip_Extractor();
	~Zip_Extractor();
	
	// Errors are reported by throwing exceptions.
	
	// Close current file and open ZIP file. True if file is ZIP archive.
	// Keeps pointer to reader until close().
	bool open( File_Reader* );
	
	// Access next item in archive. Result is false if no more items.
	// After opening or rewinding archive, this must be called to get
	// the first item.
	bool next();
	
	// Information about current item
	struct info_t {
		char name [256];
		unsigned long date; // DOS format, 0 if unavailable
		bool is_file;
		long size;
	};
	const info_t& info() const;
	
	// Extract first 'count' bytes of file info 'buf
	void extract( void* buf, long count );
	
	// Go back to beginning of archive
	void rewind();
	
	// Close archive
	void close();
	
	// End of public interface
private:
	struct entry_t;
	char* catalog;
	File_Reader* in;
	info_t info_;
	long offset;
	bool advance_entry;
};

inline const Zip_Extractor::info_t& Zip_Extractor::info() const {
	return info_;
}
inline void Zip_Extractor::rewind() {
	offset = 0;
	advance_entry = false;
}
#endif

