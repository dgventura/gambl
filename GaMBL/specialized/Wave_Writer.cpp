
// Game_Music_Box 0.5.2. http://www.slack.net/~ant/game-music-box

#include "Wave_Writer.h"

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

Wave_Writer::Wave_Writer( Mac_File* f ) : file( f ), buf( buf_size ) {
	buf_pos = header_size;
	sample_count_ = 0;
	file->set_cached( false );
}

void Wave_Writer::flush() {
	if ( buf_pos ) {
		file->write( &buf [0], buf_pos );
		buf_pos = 0;
	}
}

void Wave_Writer::write( const sample_t* in, long remain )
{
	sample_count_ += remain;
	while ( remain )
	{
		if ( buf_pos >= buf_size )
			flush();
		
		long n = (buf_size - buf_pos) / sizeof (sample_t);
		if ( n > remain )
			n = remain;
		remain -= n;
		
		// convert to lsb first format
		char* p = &buf [buf_pos];
		while ( n-- ) {
			int s = *in++;
			*p++ = char (s);
			*p++ = char (s >> 8);
		}
		
		buf_pos = p - &buf [0];
		assert( buf_pos <= buf_size );
	}
}

void Wave_Writer::trim( long count )
{
	flush();
	
	sample_count_ = count;
	file->seek( header_size + count * sizeof (sample_t) );
}

void Wave_Writer::finish( long sample_rate )
{
	flush();
	file->set_size( file->tell() );
	
	const int chan_count = 2;
	const long rate = sample_rate;
	long ds = sample_count_ * sizeof (sample_t);
	long rs = header_size - 8 + ds;
	int frame_size = chan_count * sizeof (sample_t);
	long bps = rate * frame_size;
	unsigned char header [header_size] = {
		'R','I','F','F',
		static_cast<unsigned char>(rs),static_cast<unsigned char>(rs>>8),           // length of rest of file
		static_cast<unsigned char>(rs>>16),static_cast<unsigned char>(rs>>24),
		'W','A','V','E',
		'f','m','t',' ',
		0x10,0,0,0,         // size of fmt chunk
		1,0,                // uncompressed format
		chan_count,0,       // channel count
		static_cast<unsigned char>(rate),static_cast<unsigned char>(rate >> 8),     // sample rate
		static_cast<unsigned char>(rate>>16),static_cast<unsigned char>(rate>>24),
		static_cast<unsigned char>(bps),static_cast<unsigned char>(bps>>8),         // bytes per second
		static_cast<unsigned char>(bps>>16),static_cast<unsigned char>(bps>>24),
		static_cast<unsigned char>(frame_size),0,       // bytes per sample frame
		16,0,               // bits per sample
		'd','a','t','a',
		static_cast<unsigned char>(ds),static_cast<unsigned char>(ds>>8),static_cast<unsigned char>(ds>>16),static_cast<unsigned char>(ds>>24)// size of sample data
		// ...              // sample data
	};
	
	// write header
	file->seek( 0 );
	file->write( header, sizeof header );
}
