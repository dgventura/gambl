
// Game_Music_Box 0.5.2. http://www.slack.net/~ant/game-music-box

#include "Gzip_Reader.h"

#include <algorithm>
#include "zutil.h"

/* Copyright (C) 2005 Shay Green. This module is free software; you
can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
library is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
more details. You should have received a copy of the GNU Lesser General
Public License along with this module; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA */

// from gzio.c
#define ASCII_FLAG   0x01 /* bit 0 set: file probably ascii text */
#define HEAD_CRC     0x02 /* bit 1 set: header CRC present */
#define EXTRA_FIELD  0x04 /* bit 2 set: extra field present */
#define ORIG_NAME    0x08 /* bit 3 set: original file name present */
#define COMMENT      0x10 /* bit 4 set: file comment present */
#define RESERVED     0xE0 /* bits 5..7: reserved */

// Check gz file header. Return length of header, -1 if it's invalid, or 0 if it's not
// a gz file.
static int check_gz_header( const unsigned char* begin, const unsigned char* end )
{
	const unsigned char* p = begin;
	
	if ( end - p < 10 )
		return 0;
	
	if ( *p++ != 0x1F )
		return 0;
	if ( *p++ != 0x8B )
		return 0;
	
	int method = *p++;
	if ( method != Z_DEFLATED )
		return -1;
	
	int flags = *p++;
	if ( flags & RESERVED )
		return -1;
	
	p += 6; // skip time, xflags, OS code
	
	if ( flags & EXTRA_FIELD ) {
		unsigned skip = 2 + p [1] * 0x100 + p [0];
		if ( end - p < skip )
			return -1;
		p += skip;
	}
	
	if ( flags & ORIG_NAME )
		while ( p != end && *p++ ) { }
		
	if ( flags & COMMENT )
		while ( p != end && *p++ ) { }
	
	if ( flags & HEAD_CRC ) {
		if ( end - p < 2 )
			return -1;
		p += 2;
	}
	
	if ( p >= end )
		return -1;
	
	return p - begin;
}

static const z_stream empty_zbuf = { 0 };

static void throw_zlib_error( int code )
{
	if ( code != Z_OK )
	{
		const char* str = zError( code );
		if ( code == Z_MEM_ERROR )
			str = "Out of memory";
		
		throw_error( str ? str : "Gzip error" );
	}
}
	
void write_gzip_file( const void* in, long size, Data_Writer& out )
{
	static z_stream zs_init;
	
	z_stream zs = zs_init;
	runtime_array<Bytef> buf( 16 * 1024L );
	
	// -MAX_WBITS suppresses usual header and trailer
	throw_zlib_error( deflateInit2( &zs, Z_BEST_COMPRESSION, Z_DEFLATED,
			-MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY ) );
	
	static const unsigned char header [10] = {
		0x1f, 0x8b, Z_DEFLATED,
		0, // flags
		0, 0, 0, 0, // time
		0, // xflags
		OS_CODE
	};
	out.write( header, sizeof header );
	
	zs.next_in = (Bytef*) in;
	zs.avail_in = size;
	
	while ( true )
	{
		zs.next_out = buf.begin();
		zs.avail_out = buf.size();
		
		int result = deflate( &zs, Z_FINISH );
		if ( result != Z_STREAM_END )
			throw_zlib_error( result );
		
		out.write( buf.begin(), buf.size() - zs.avail_out );
		
		if ( result == Z_STREAM_END )
			break;
	}
	
	throw_zlib_error( deflateEnd( &zs ) );
	
	assert( zs.total_in == size );
	
	unsigned long crc = crc32( 0, Z_NULL, 0 );
	crc = crc32( crc, (Bytef*) in, size );
	
	unsigned char trailer [8] = {
		static_cast<unsigned char>(crc), static_cast<unsigned char>(crc >> 8),
        static_cast<unsigned char>(crc >> 16), static_cast<unsigned char>(crc >> 24),
		static_cast<unsigned char>(size), static_cast<unsigned char>(size >> 8),
        static_cast<unsigned char>(size >> 16), static_cast<unsigned char>(size >> 24)
	};
	out.write( trailer, sizeof trailer );
}

void inflate_mem_gzip( const void* in_, long in_size, runtime_array<char>& out )
{
	unsigned char* in = (unsigned char*) in_;
	
	int header_size = check_gz_header( in, in + in_size );
	if ( !header_size ) {
		out.resize( in_size );
		std::memcpy( out.begin(), in, out.size() );
		return;
	}
	
	const int trailer_size = 8;
	unsigned char* trailer = in + in_size - trailer_size;
	unsigned long correct_crc = trailer [3] * 0x1000000L + trailer [2] * 0x10000L +
			trailer [1] * 0x100L + trailer [0];
	long size = trailer [7] * 0x1000000L + trailer [6] * 0x10000L +
			trailer [5] * 0x100L + trailer [4];
	out.resize( size );
	
	// init zbuf
	z_stream zbuf = empty_zbuf;
	
	zbuf.next_in = in + header_size;
	zbuf.avail_in = in_size - header_size - trailer_size;
	
	zbuf.next_out = (unsigned char*) out.begin();
	zbuf.avail_out = out.size();
	
	throw_zlib_error( inflateInit2( &zbuf, -MAX_WBITS ) );
	int result = inflate( &zbuf, Z_FINISH );
	debug_if_error( inflateEnd( &zbuf ) );
	if ( result != Z_STREAM_END )
		throw_zlib_error( result );
	
	unsigned long crc = crc32( 0, Z_NULL, 0 );
	crc = crc32( crc, (unsigned char*) out.begin(), out.size() );
	if ( crc != correct_crc )
		throw_error( "File data is corrupt (bad gzip CRC)" );
}

void Gzip_Reader::throw_zlib_error( int err )
{
	if ( err )
		throw_file_error( zError( err ), path );
}

Gzip_Reader::Gzip_Reader( const FSRef& path_ ) :
	path( path_ ),
	file( path_ ),
	buf( 32 * 1024L )
{
	// clear zbuf fields
	zbuf = empty_zbuf;
	
	// file size
	long size = file.size();
	raw_remain = size;
	
	// read first buffer and check for gzip header
	fill_buf( 2048 ); // read less the first time
	is_deflated_ = false;
	int header_size = check_gz_header( buf.begin(), buf.begin() + zbuf.avail_in );
	if ( header_size )
	{
		is_deflated_ = true;
		
		// read size
		assert( file.tell() == zbuf.avail_in );
		file.seek( size - 8 );
		unsigned char trailer [8];
		file.read( trailer, sizeof trailer );
		file.seek( zbuf.avail_in );
		correct_crc = trailer [3] * 0x1000000L + trailer [2] * 0x10000L +
				trailer [1] * 0x100L + trailer [0];
		size = trailer [7] * 0x1000000L + trailer [6] * 0x10000L +
				trailer [5] * 0x100L + trailer [4];
		raw_remain -= 8 - 1; // inflate requires an extra byte at the end
		
		// init zbuf
		zbuf.next_in += header_size;
		zbuf.avail_in -= header_size;
		throw_zlib_error( inflateInit2( &zbuf, -MAX_WBITS ) );
		
		crc = crc32( 0, Z_NULL, 0 );
	}
	
	remain_ = size;
}

// deflate/copy (if uncompressed) as much as possible using zbuf in/out parameters
void Gzip_Reader::deflate()
{
	if ( !zbuf.avail_out )
		return;
	
	if ( is_deflated_ )
	{
		int result = inflate( &zbuf, Z_SYNC_FLUSH );
		if ( result != Z_STREAM_END )
		{
			if ( result != Z_MEM_ERROR && raw_remain == 0 && zbuf.avail_out > 0 )
				throw_error( "Gzip error (file is probably corrupt)" );
			throw_zlib_error( result );
		}
	}
	else {
		long n = std::min( zbuf.avail_in, zbuf.avail_out );
		std::memcpy( zbuf.next_out, zbuf.next_in, n );
		zbuf.next_in += n;
		zbuf.avail_in -= n;
		zbuf.next_out += n;
		zbuf.avail_out -= n;
	}
}

void Gzip_Reader::fill_buf( long max_read )
{
	zbuf.next_in = &buf [0];
	zbuf.avail_in = std::min<long>( max_read, raw_remain );
	raw_remain -= zbuf.avail_in;
	file.read( zbuf.next_in, zbuf.avail_in );
}

long Gzip_Reader::read_avail( void* p, long n )
{
	zbuf.next_out = static_cast<unsigned char*> (p);
	zbuf.avail_out = n;
	while ( zbuf.avail_out )
	{
		if ( !zbuf.avail_in )
			fill_buf( buf.size() );
		
		long prev_avail = zbuf.avail_out;
		deflate();
		if ( zbuf.avail_out == prev_avail )
			throw_file_error( "Unexpected end of file", path );
	}
	
	remain_ -= n;
	assert( remain_ >= 0 );
	if ( is_deflated_ )
	{
		crc = crc32( crc, (Bytef*) p, n );
		if ( remain_ == 0 && crc != correct_crc )
			throw_file_error( "File data is corrupt (bad gzip CRC)", path );
	}
	
	return n;
}

Gzip_Reader::~Gzip_Reader()
{
	if ( is_deflated_ )
		debug_if_error( inflateEnd( &zbuf ) );
}
	
long Gzip_Reader::remain()
{
	return remain_;
}

