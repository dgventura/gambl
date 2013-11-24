
/* Packed_Spc 0.1.2. http://www.slack.net/~ant/libs/ */

#include "unpack_spc.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>

#include "zlib/zlib.h"

/* Library Copyright (C) 2004 Shay Green. This library is free software;
you can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
library is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details. You should have received a copy of the GNU Lesser General
Public License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA */

enum { ram_size = 0x10000 };

struct spc_file_t {
	unsigned char header [0x100];
	unsigned char ram [ram_size];
	unsigned char dsp [0x80];
};

typedef struct spc_file_t spc_file_t;
typedef struct packed_spc_t packed_spc_t;

int is_packed_spc( const void* spc, long size ) {
	return size >= sizeof (packed_spc_t) &&
			((packed_spc_t*) spc)->header [0xD8] & 0x01 &&
			strncmp( ((packed_spc_t*) spc)->header, "SNES-SPC700 Sound File Data", 27 ) == 0;
}

const char* spc_shared_filename( const void* spc, long size ) {
	return is_packed_spc( spc, size ) ?
			((packed_spc_t*) spc)->shared_filename : NULL;
}

const char* read_packed_spc( const void* spc, long spc_size, const char* shared_path,
		void* out_buf, long* out_size )
{
	const char* error = NULL;
	long buf_alloc = 0;
	long shared_size = 0;
	char* buf = 0;
	long count = 0;
	
	gzFile shared = gzopen( shared_path, "rb" );
	if ( !shared )
		return "Couldn't open shared file";
	do {
		if ( shared_size == buf_alloc ) {
			void* new_buf = realloc( buf, buf_alloc += 64 * 1024L );
			if ( !new_buf ) {
				error = "Out of memory";
				goto error;
			}
			buf = (char*) new_buf;
		}
		count = gzread( shared, buf + shared_size, buf_alloc - shared_size );
		shared_size += count;
	}
	while ( count );
	
	error = unpack_spc( spc, spc_size, buf, shared_size, out_buf, out_size );
	
error:
	free( buf );
	gzclose( shared );
	
	return error;
}

const char* unpack_spc( const void* spc, long spc_size, const void* shared,
		long shared_size, void* out_, long* out_size )
{
	const packed_spc_t* const in = (packed_spc_t*) spc;
	struct spc_file_t* const out = (struct spc_file_t*) out_;
	
	/* basic checks */
	assert( *out_size >= sizeof (packed_spc_t) );
	assert( sizeof (spc_file_t) == 0x100 + ram_size + 0x80 );
	assert( sizeof (packed_spc_t) == sizeof (spc_file_t) );
	
	if ( !is_packed_spc( spc, spc_size ) )
		return "Not a packed SPC file";
	
	if ( in->format != 'P' )
		return "Unknown SPC packing method";
	
	/* common data */
	memcpy( out->header, in->header, sizeof out->header );
	out->header [0xD8] &= ~0x01; /* clear packed flag */
	memcpy( out->dsp, in->dsp,
			((spc_size < *out_size) ? spc_size : *out_size) - offsetof (spc_file_t,dsp) );
	*out_size = spc_size;
	
	/* unpack */
	{
		long pos = 0;
		const unsigned char* cmp = in->cmp;
		while ( pos < ram_size )
		{
			void* dest = &out->ram [pos];
			
			/* get block type and (output) size */
			int type = *cmp++;
			int size = *cmp++;
			size += *cmp++ * 0x100;
			
			pos += size;
			if ( pos > ram_size )
				return "Packed SPC data went past end";
			
			switch ( type )
			{
				/* shared */
				case packed_spc_shared: {
					long offset = *cmp++;
					offset += *cmp++ * 0x100;
					offset += *cmp++ * 0x10000;
					if ( offset + size > shared_size )
						return "Packed SPC block went past end of shared data file";
					memcpy( dest, (const char*) shared + offset, size );
					break;
				}
				
				/* unique */
				case packed_spc_unique:
					memcpy( dest, cmp, size );
					cmp += size;
					break;
				
				/* fill */
				case packed_spc_0x00:
				case packed_spc_0xFF:
					memset( dest, type == packed_spc_0xFF ? 0xFF : 0, size );
					break;
				
				default:
					return "Unknown block type in packed SPC";
			}
		}
		if ( pos != ram_size || cmp > in->dsp )
			return "Unpacked SPC data came out to wrong size";
	}
	
	/* verify ram checksum */
	{
		unsigned checksum = in->checksum [1] * 0x100 + in->checksum [0];
		unsigned sum = 0;
		unsigned i;
		for ( i = 0; i < ram_size; i++ )
			sum += out->ram [i];
		if ( (sum & 0xffff) != checksum )
			return "Checksum of unpacked SPC data differs";
	}
	
	return NULL; /* success */
}

