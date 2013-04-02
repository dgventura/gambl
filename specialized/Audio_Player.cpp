
// Game_Music_Box 0.5.2. http://www.slack.net/~ant/game-music-box

#include "Audio_Player.h"

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

#if GMB_COMPILE_AUDIO
static void SndDoImmediateChk( SndChannelPtr chan, unsigned cmd, int p1 = 0, long p2 = 0 )
{
	SndCommand command;
	command.cmd = cmd;
	command.param1 = p1;
	command.param2 = p2;
	throw_if_error( SndDoImmediate( chan, &command ) );
}
#endif

Audio_Player::Audio_Player()
{
	vm_holder.hold( this, sizeof *this );

#if GMB_COMPILE_AUDIO
	static SndCallBackUPP chan_callback_upp =
			throw_if_null( NewSndCallBackUPP( chan_callback ) );
	
	chan = NULL;
	throw_if_error( SndNewChannel( &chan, sampledSynth, initStereo, chan_callback_upp ) );
	
	std::memset( &snd_header, 0, sizeof snd_header );
	snd_header.encode = extSH;
	snd_header.baseFrequency = kMiddleC;
	snd_header.samplePtr = NULL;
#endif
}

Audio_Player::~Audio_Player()
{
	stop();
#if GMB_COMPILE_AUDIO
	debug_if_error( SndDisposeChannel( chan, false ) );
#endif
}

void Audio_Player::set_gain( double v )
{
#if GMB_COMPILE_AUDIO
	SndDoImmediateChk( chan, volumeCmd, 0, int (v * 0x100) * 0x10001 );
#endif
}

double Audio_Player::hw_sample_rate() const
{
	unsigned long rate = 0;
#if GMB_COMPILE_AUDIO
    if ( debug_if_error( SndGetInfo( chan, siSampleRate, &rate ) ) )
		rate = 0;
#endif
	return 1.0 / 0x10000 * rate;
}

void Audio_Player::setup( long sample_rate, bool stereo, callback_t func, void* data )
{
#if GMB_COMPILE_AUDIO
	require( !snd_header.samplePtr );
	callback = func;
	callback_data = data;
	snd_header.numChannels = stereo + 1;
	snd_header.sampleRate  = sample_rate * 0x10000;
	snd_header.sampleSize  = 16;
#endif
}

void Audio_Player::play_buffer( const sample_t* buf, int count )
{
#if GMB_COMPILE_AUDIO
	require( !snd_header.samplePtr );
	snd_header.samplePtr = reinterpret_cast<Ptr> (const_cast<sample_t*> (buf));
	snd_header.numFrames = count >> (snd_header.numChannels - 1);
	
	SndCommand cmd;
	cmd.param1 = 0;
	
	cmd.cmd = bufferCmd;
	cmd.param2 = reinterpret_cast<long> (&snd_header);
	debug_if_error( SndDoCommand( chan, &cmd, false ) );
	
	cmd.cmd = callBackCmd;
	cmd.param2 = reinterpret_cast<long> (this);
	debug_if_error( SndDoCommand( chan, &cmd, false ) );
#endif
}

#if GMB_COMPILE_AUDIO
pascal void Audio_Player::chan_callback( SndChannelPtr chan, SndCommand* cmd )
{
	Audio_Player* player = reinterpret_cast<Audio_Player*> (cmd->param2);
	assert( player->snd_header.samplePtr );
	player->snd_header.samplePtr = NULL;
	player->callback( player->callback_data );
}
#endif

void Audio_Player::stop()
{
#if GMB_COMPILE_AUDIO
	SndDoImmediateChk( chan, flushCmd );
	SndDoImmediateChk( chan, quietCmd );
	snd_header.samplePtr = NULL;
#endif
}

