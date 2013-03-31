
// Reader for gzipped (and normal) files, more streamlined than gzopen(), gzread(), etc.

// Game_Music_Box 0.5.2. Copyright (C) 2005 Shay Green. GNU LGPL license.

#ifndef GZIP_READER_H
#define GZIP_READER_H

#include "common.h"
#include "runtime_array.h"
#include "abstract_file.h"
#include "file_util.h"
#include "zlib.h"

// Inflate gzip file that's been read into memory (if uncompressed, just copy to out).
void inflate_mem_gzip( const void*, long size, runtime_array<char>& out );

// Deflate data into gzip file
void write_gzip_file( const void*, long size, Data_Writer& out );

class Gzip_Reader : public Data_Reader {
public:
	explicit Gzip_Reader( const FSRef& );
	~Gzip_Reader();
	
	bool is_deflated() const;
	long remain() const;
	
	// Reports errors with exceptions
	long read_avail( void*, long );
	
private:
	runtime_array<unsigned char> buf;
	Mac_File_Reader file;
	FSRef path;
	z_stream zbuf;
	long raw_remain;
	long remain_;
	unsigned long correct_crc;
	unsigned long crc;
	bool is_deflated_;
	
	void fill_buf( long max_read );
	void deflate();
	void throw_zlib_error( int err );
};

inline bool Gzip_Reader::is_deflated() const {
	return is_deflated_;
}

#endif

