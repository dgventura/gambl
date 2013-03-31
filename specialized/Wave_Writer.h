
// Simple wave sound file writer

// Game_Music_Box 0.5.2. Copyright (C) 2005 Shay Green. GNU LGPL license.

#ifndef WAVE_WRITER_H
#define WAVE_WRITER_H

#include "common.h"
#include "runtime_array.h"
#include "file_util.h"

class Wave_Writer {
public:
	// Write wave file to already-open file
	Wave_Writer( Mac_File* );
	
	typedef short sample_t;
	
	// Write 'count' samples from 'buf'
	void write( const sample_t* buf, long count );
	
	// Total samples written
	long sample_count() const;
	
	// Remove any samples beyond sample 'n'
	void trim( long n );
	
	// Flush buffer and write wave header
	void finish( long sample_rate );
	
private:
	enum { header_size = 0x2C };
	enum { buf_size = 512 * 1024L };
	Mac_File* file;
	long sample_count_;
	long buf_pos;
	runtime_array<char> buf;
	
	void flush();
};

inline long Wave_Writer::sample_count() const {
	return sample_count_;
}

#endif

