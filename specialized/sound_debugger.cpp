
// Game_Music_Box 0.5.2. http://www.slack.net/~ant/game-music-box

#include "sound_debugger.h"

#include <algorithm>
#include "Wave_Writer.h"
#include "file_util.h"
#include "thread_util.h"

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

#include "source_begin.h"

class Sound_Debugger {
public:
	
	Sound_Debugger();
	~Sound_Debugger();
	
	typedef Wave_Writer::sample_t sample_t;
	void write( const sample_t*, long count );
	
private:
	Mac_File file;
	Wave_Writer wave;
	enum { buf_size = 512 * 1024L };
	runtime_array<sample_t> buf;
	volatile long write_pos;
	volatile long read_pos;
	Event_Loop_Timer task;
	
	long written() const;
	void flush();
	static void flush_( void* );
};

static FSRef make_debug_wave() {
	static HFSUniStr255 name = { 9, L"debug.wav" };
	FSRef dir = get_parent( get_bundle_fsref() );
	FSRef file;
	if ( FSMakeFSRefExists( dir, name, &file ) )
		FSDeleteObject( &file );
	create_file( dir, name, 'WAVE', 'Nqst' );
	return FSMakeFSRefChk( dir, name );
}
	
Sound_Debugger::Sound_Debugger() :
	file( make_debug_wave() ),
	wave( &file ),
	buf( buf_size )
{
	write_pos = 0;
	read_pos = 0;
	task.set_callback( flush_, this );
	task.install( 0.5 );
}

Sound_Debugger::~Sound_Debugger() {
	flush();
	wave.finish( 44100 );
}
		
long Sound_Debugger::written() const {
	return (write_pos + buf_size - read_pos) % buf_size;
}

void Sound_Debugger::write( const sample_t* in, long count )
{
	long pos = 0;
	while ( true )
	{
		long n = buf_size - 1 - written();
		n = std::min( n, long (buf_size - write_pos) );
		if ( !n )
			dprintf( "Sound debugger buffer is full; some output will be lost." );
		n = std::min( n, count - pos );
		
		if ( !n )
			break;
		
		std::memcpy( buf.begin() + write_pos, in + pos, n * sizeof *in );
		write_pos = (write_pos + n) % buf_size;
		pos += n;
	}
}

void Sound_Debugger::flush()
{
	while ( true )
	{
		long n = written();
		n = std::min( n, long (buf_size - read_pos) );
		
		if ( !n )
			break;
		
		wave.write( buf.begin() + read_pos, n );
		read_pos = (read_pos + n) % buf_size;
	}
}

void Sound_Debugger::flush_( void* self ) {
	static_cast<Sound_Debugger*> (self)->flush();
}

static Sound_Debugger& sound_debugger() {
	static Sound_Debugger sd;
	return sd;
}

void open_sound_debugger() {
	sound_debugger();
}

void write_sound_debugger( const BOOST::int16_t* p, long s ) {
	sound_debugger().write( p, s );
}

