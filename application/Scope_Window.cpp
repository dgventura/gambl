
// Game_Music_Box 0.5.2. http://www.slack.net/~ant/game-music-box

#include "Scope_Window.h"

#include "prefs.h"

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

// The scope renders the waveform to a two-color offscreen gworld and copies
// the minimum possible center strip to the window.

const int stereo = 2; // two channels

// Scope_Window

Scope_Window::Scope_Window()
{
	buffer = NULL;
	
	timer.set_callback( update, this );
	
	Carbon_Window::make( kDocumentWindowClass, kWindowStandardHandlerAttribute |
			kWindowCloseBoxAttribute | kWindowFullZoomAttribute );
	Carbon_Window::set_title( "Sound Scope" );
	
	set_zoom( prefs.scope.zoomed );
	
	RepositionWindow( window(), NULL, kWindowCenterOnMainScreen );
	restore_top_left( prefs.scope.window_pos );
}

Scope_Window::~Scope_Window()
{
	prefs.scope.window_pos = top_left();
	prefs.scope.visible = visible();
}

void Scope_Window::set_zoom( bool b )
{
	prefs.scope.zoomed = b;
	int width = b ? 640 : 320;
	scope.resize( width, 256 );
	SizeWindow( window(), width, b ? 256 : 256 - 96, true );
	//redraw( 0, 0 );
	update_( true );
}

bool Scope_Window::handle_command( const HICommandExtended& cmd )
{
	switch ( cmd.commandID )
	{
		case 'Clos':
			show( false );
			return true;
		
		case 'Zoom':
			set_zoom( !prefs.scope.zoomed );
			return true;
	}
	
	return Carbon_Window::handle_command( cmd );
}

void Scope_Window::clear( bool flatline )
{
	scope.clear();
	
	if ( flatline ) {
		static short zero [2];
		scope.render( zero, 0 );
	}
	
	if ( visible() ) {
		redraw( 0, 0 );
		flush_window( window() );
	}
}

void Scope_Window::draw( int min, int max )
{
	scope.draw( point( 0, prefs.scope.zoomed ? 0 : -48 ), min, max );
}

void Scope_Window::draw_content()
{
	draw( 0, 0 );
}

void Scope_Window::show( bool b )
{
	Carbon_Window::show( b );
	
	if ( b )
		timer.install( 1.0 / 35 );
	else
		timer.remove();
}

const short* Scope_Window::get_buffer( bool update_only )
{
	const short* p = buffer;
	if ( !update_only )
		buffer = NULL;
	return p;
}

void Scope_Window::redraw( int min, int max )
{
	GrafPtr port;
	GetPort( &port );
	SetPort( GetWindowPort( window() ) );
	draw( min, max );
	SetPort( port );
}

void Scope_Window::update_( bool update_only )
{
	const short* buf = get_buffer( update_only );
	if ( buf )
	{
		short min = 0;
		short max = 0;
		scope.render( buf, prefs.scope.zoomed ? 2 : 4, &min, &max );
		redraw( min, max );
		flush_window( window() );
	}
}

void Scope_Window::update( void* self )
{
	static_cast<Scope_Window*> (self)->update_( false );
}

// Scope_Renderer

Scope_Renderer::Scope_Renderer() : bounds( 0, 0, 256, 128 )
{
	color_table = GetCTable( 200 );
	gworld = NULL;
	sample_shift = 10;
	resize( 256, 128 );
}

Scope_Renderer::~Scope_Renderer()
{
	DisposeGWorld( gworld );
}

void Scope_Renderer::resize( int width, int height )
{
	if ( gworld )
	{
		// to do: update gworld rather than re-creating
		DisposeGWorld( gworld );
		gworld = NULL;
	}
	
	old_buf.clear();
	old_buf.resize( width );
	old_min = 0;
	old_max = 0;
	
	bounds.right = width;
	bounds.bottom = height;
	for ( sample_shift = 6; sample_shift < 14; sample_shift++ )
		if ( ((0x7fff * 2) >> sample_shift) < height )
			break;
	sample_shift++;
	
	throw_if_error( NewGWorld( &gworld, 1, bounds, color_table, NULL, 0 ) );
	
	PixMapHandle pm = GetGWorldPixMap( gworld );
	LockPixels( pm );
	
	pixels = (byte*) GetPixBaseAddr( pm );
	row_bytes = GetPixRowBytes( pm );
	
	clear();
}

void Scope_Renderer::clear()
{
	CGrafPtr port;
	GDHandle device;
	GetGWorld( &port, &device );
	SetGWorld( gworld, NULL );
	PaintRect( bounds );
	SetGWorld( port, device );
	std::memset( old_buf.begin(), 0, old_buf.size() );
	old_min = 0;
	old_max = bounds.height() - 1;
}

void Scope_Renderer::draw( const point& top_left, int min, int max )
{
	rect src = bounds;
	if ( min | max )
	{
		src.top = min;
		src.bottom = max;
	}
	rect dest = src;
	OffsetRect( &dest, top_left.h, top_left.v );
	
	if ( false )
	{
		// show the rectangle copied, for debugging
		rect r = dest;
		r.top--;
		r.bottom++;
		EraseRect( r );
	}
	
	GrafPtr port;
	GetPort( &port );
	CopyBits( GetPortBitMapForCopyBits( gworld ), GetPortBitMapForCopyBits( port ),
			src, dest, srcCopy, NULL );
}

#if defined (__MWERKS__) && OPTIMIZE
	#pragma peephole on
	#pragma global_optimizer on
	#pragma optimization_level 4
	#pragma scheduling 750
#else
	#include "enable_optimizer.h"
#endif

void Scope_Renderer::render( const short* in, int in_step, short* min_out, short* max_out )
{
	int const row_bytes = this->row_bytes;
	byte* old_pos = old_buf.begin();
	byte* out = pixels;
	int shift = sample_shift;
	int old_erase = *old_pos;
	int old_draw = (0x7fff * 2 - in [0] - in [1]) >> shift;
	
	int min = INT_MAX;
	int max = 0;
	
	for ( int n = old_buf.size() / 8; n--; )
	{
		int bit = 0x80;
		for ( int n = 8; n--; bit >>= 1 )
		{
			// Line drawing/erasing starts at previous sample and ends one short of
			// current sample, except when previous and current are the same.
			
			// Extra read on the last iteration of line loops will always be at the
			// height of the next sample, and thus within the gworld bounds.
			
			// Erase old line
			{
				int delta = *old_pos - old_erase;
				int offset = old_erase * row_bytes;
				old_erase += delta;
				
				int pixel = out [offset];
				
				int next_line = row_bytes;
				if ( delta < 0 ) {
					delta = -delta;
					next_line = -row_bytes;
				}
				
				do
				{
					out [offset] = pixel | bit;
					offset += next_line;
					pixel = out [offset];
				}
				while ( delta-- > 1 );
			}
			
			// Draw new line and put in old_buf
			{
				int sample = (0x7fff * 2 - in [0] - in [1]) >> shift;
				in += in_step;
				assert( (unsigned) sample < bounds.height() );
				
				int delta = sample - old_draw;
				int offset = old_draw * row_bytes;
				old_draw += delta;
				
				int next_line = row_bytes;
				if ( delta < 0 ) {
					delta = -delta;
					next_line = -row_bytes;
				}
				
				int pixel = out [offset];
				*old_pos++ = sample;
				int neg_bit = ~bit;
				
				// min/max updating can be interleved anywhere
				
				if ( sample < min )
					min = sample;
				
				do
				{
					out [offset] = pixel & neg_bit;
					offset += next_line;
					pixel = out [offset];
				}
				while ( delta-- > 1 );
				
				if ( sample > max )
					max = sample;
			}
		}
		++out;
	}
	
	if ( min_out )
		*min_out = (min < old_min) ? min : old_min;
	old_min = min;
	
	if ( max_out )
		*max_out = (max > old_max ? max : old_max) + 1;
	old_max = max;
}

