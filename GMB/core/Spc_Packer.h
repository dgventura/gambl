
// Super Nintendo SPC music file packer

// Packed_Spc 0.1.2. Copyright (C) 2004-2005 Shay Green. GNU LGPL license.

#ifndef SPC_PACKER_H
#define SPC_PACKER_H

#include <string.h>
#include "abstract_file.h"

// For writing files. Should compress data into a gzip file using zlib.
typedef Data_Writer Spc_Writer;

class Spc_Packer {
public:
	Spc_Packer();
	~Spc_Packer();
	
	// All functions return NULL on success, otherwise an error string.
	
	// Initialize packer for use with a shared data file of specified name.
	const char* init( const char* shared_filename );
	
	// Preserve echo buffer contents rather than clearing it. Results in larger files.
	void preserve_echo_buffer() { preserve_echo_buffer_ = true; }
	
	// Usage: add each SPC file with add_spc(), then pack each with pack_spc(),
	// finally write_shared().
	
	// Add SPC file data to set of SPCs to compress.
	const char* add_spc( const void* data, long size );
	
	// Pack next file using writer to generate output file. Files are packed in the
	// order added.
	const char* pack_spc( Spc_Writer& );
	
	// Write shared file.
	const char* write_shared( Spc_Writer& );
	
	
	// End of public interface
	
	typedef unsigned char byte;
	
	enum { block_size = 256 };
	enum { ram_size = 0x10000 };
	enum { max_checksum = 255 * block_size };
	typedef unsigned short checksum_t;
private:
	
	struct file_t {
		checksum_t* checksums;
		long size;
		long file_size;
		file_t* next;
		checksum_t checksums_ [ram_size / block_size]; // not used for shared file
		byte file_data [0x100]; // not used for shared file
		byte data [ram_size + 0x1000];
	};
	
	file_t* files;
	file_t* shared;
	long shared_reserved;
	
	char shared_filename [64];
	bool preserve_echo_buffer_;
	
	class Checksum_Map {
		char bits [max_checksum / 8 + 1];
		byte data [max_checksum + 1];
		enum { no_checksum = 0xFF };
	public:
		Checksum_Map() {
			memset( bits, 0, sizeof bits );
		}
		int test( int n, int d ) const {
			return bits [n >> 3] & (1 << (n & 7)) && (data [n] == d || data [n] == no_checksum);
		}
		void set( int n, int d ) {
			int bit = 1 << (n & 7);
			if ( !(bits [n >> 3] & bit) ) {
				data [n] = d;
				bits [n >> 3] |= bit;
			}
			else if ( data [n] != d ) {
				data [n] = no_checksum;
			}
		}
	};
	
	Checksum_Map shared_map;
	Checksum_Map checksum_map;
	
	byte temp_buf [ram_size + 0x100];
	
	struct result_t {
		file_t* file;
		int skip;
		long offset;
		int size;
	};
	
	long add_shared( const byte* data, unsigned size );
	bool match_data( unsigned sum, byte* begin, byte* pos, byte* end, file_t* file, result_t* );
	byte* match_blocks( byte* out, byte* begin, byte* end, int phase );
	
	enum { shared_reserve = ram_size + 0x100 };
	enum { shared_checksum_reserve = shared_reserve / block_size + 1 };
	bool expand_shared();
};

#endif

