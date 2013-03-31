
// Game_Music_Box 0.5.2. http://www.slack.net/~ant/game-music-box

#include "mac_controls.h"

//#include <ControlDefinitions.h>

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

const ControlFontStyleRec default_text_style = { 0 };

Row_Placer::Row_Placer( int width, point const& tl ) :
	m_bounds( tl.h, tl.v, tl.h + width, tl.v ),
	spacing( 4 )
{
}

rect Row_Placer::place( int height, int width )
{
	if ( !width )
		width = m_bounds.width();
	if ( m_bounds.height() != 0 )
		m_bounds.bottom += spacing;
	rect result( m_bounds.left, m_bounds.bottom,
			m_bounds.left + width, m_bounds.bottom + height );
	m_bounds.bottom += height;
	return result;
}

Row_Filler::Row_Filler( rect const& bounds, int s ) :
	m_bounds( bounds ),
	spacing( s )
{
}
	
rect Row_Filler::place( int height ) {
	if ( !height )
		height = m_bounds.height();
	
	rect result;
	result.top = m_bounds.top + (m_bounds.height() - height) / 2;
	result.bottom = result.top + height;
	result.right = m_bounds.right;
	result.left = m_bounds.left;
	return result;
}

rect Row_Filler::place_left( int width, int height ) {
	rect result( place( height ) );
	result.right = result.left + width;
	m_bounds.left = result.right + spacing;
	return result;
}

rect Row_Filler::place_right( int width, int height ) {
	rect result( place( height ) );
	result.left = result.right - width;
	m_bounds.right = result.left - spacing;
	return result;
}

rect Row_Filler::remain( int height ) {
	rect result( place( height ) );
	m_bounds.right = m_bounds.left;
	return result;
}

void Control::enable( bool b ) {
	enable_control( control, b );
}

void Control::show( bool b ) {
	SetControlVisibility( control, b, true );
}

void Control::set_command( long cmd ) {
	throw_if_error( SetControlCommandID( control, cmd ) );
}

void Control::set_title( const char* str ) {
	set_control_title( control, str );
}

long Value_Control::value() const {
	return GetControl32BitValue( control );
}

void Value_Control::set_value( long n ) {
	SetControl32BitValue( control, n );
}

ControlRef make_static_text( WindowRef w, const Rect& r, const char* text,
		const ControlFontStyleRec& style ) {
	ControlRef control;
	throw_if_error( CreateStaticTextControl( w, &r, __CFStringMakeConstantString( text ),
			&style, &control ) );
	return control;
}

void Static_Text::make( WindowRef w, const Rect& r, const ControlFontStyleRec& style ) {
	control = make_static_text( w, r, "", style );
}
	
void Static_Text::set_text( const char* p ) {
	set_text( p, std::strlen( p ) );
}

void Static_Text::set_text( const char* p, int len ) {
	throw_if_error( SetControlData( control, kControlEntireControl,
			kControlStaticTextTextTag, len, p ) );
	Draw1Control( control );
}

ControlRef make_push_button( WindowRef w, const Rect& r, const char* title, long cmd ) {
	ControlRef control;
	throw_if_error( CreatePushButtonControl( w, &r, __CFStringMakeConstantString( title ),
			&control ) );
	throw_if_error( SetControlCommandID( control, cmd ) );
	return control;
}

ControlRef make_cicon_button( WindowRef w, const Rect& r, int res_id, long cmd ) {
	ControlButtonContentInfo info;
	info.contentType = kControlContentCIconRes;
	info.u.resID = res_id;
	ControlRef control;
	throw_if_error( CreateIconControl( w, &r, &info, false, &control ) );
	throw_if_error( SetControlCommandID( control, cmd ) );
	return control;
}

void set_button_icon( ControlRef control, int res_id )
{
	SInt16 current = res_id ^ 1;
	GetControlData( control, kControlIconPart,
			kControlIconResourceIDTag, sizeof current, &current, NULL );
	if ( current != res_id ) {
		current = res_id;
		debug_if_error( SetControlData( control, kControlIconPart,
				kControlIconResourceIDTag, sizeof current, &current ) );
		Draw1Control( control );
	}
}

void Value_Control::make_checkbox( WindowRef w, const Rect& r, const char* title, long cmd,
		bool auto_toggle )
{
	throw_if_error( CreateCheckBoxControl( w, &r, __CFStringMakeConstantString( title ),
			0, auto_toggle, &control ) );
	set_command( cmd );
}

void Value_Control::make_radio( WindowRef w, const Rect& r, const char* title, long cmd )
{
	throw_if_error( CreateRadioButtonControl( w, &r, __CFStringMakeConstantString( title ),
			0, false, &control ) );
	set_command( cmd );
}

void set_control_title( ControlRef control, const char* str )
{
	Str255 old_title;
	GetControlTitle( control, old_title );
	Str255 new_title;
	c2pstrcpy( new_title, str );
	
	// don't change if new title is same as old (reduces flicker)
	if ( 0 != std::memcmp( old_title, new_title, old_title [0] + 1 ) )
		SetControlTitle( control, new_title );
}

void Slider_Control::make( WindowRef w, const Rect& r, int pt, callback_t cb, void* cbd )
{
	callback = cb;
	callback_data = cbd;
	static ControlActionUPP handle_slider_upp = NewControlActionUPP( handle_slider );
	
	throw_if_error( CreateSliderControl( w, &r, 0, 0, 0x10000,
			(pt < 0 ? kControlSliderPointsUpOrLeft :
			pt > 0 ? kControlSliderPointsDownOrRight : kControlSliderDoesNotPoint),
			(pt < 0 ? -pt : pt), cb != 0, cb ? handle_slider_upp : NULL, &control ) );
	if ( cb )
		SetControlReference( control, reinterpret_cast<long> (this) );
}
	
pascal void Slider_Control::handle_slider( ControlRef control, ControlPartCode part ) {
	Slider_Control* self = reinterpret_cast<Slider_Control*> (GetControlReference( control ));
	self->callback( self->callback_data, self, (part == kControlPageUpPart ? -1 :
			part == kControlPageDownPart ? 1 : 0) );
}

double Slider_Control::fraction() const {
	return value() / (double) 0x10000;
}

void Slider_Control::set_fraction( double d ) {
	set_value( d * 0x10000 );
}

ControlRef make_titled_slider( Slider_Control& slider, WindowRef win, const Rect& bounds,
		const char* title, int title_width, int pointer_ticks,
		Slider_Control::callback_t callback, void* data )
{
	Rect r = bounds;
	
	r.right = r.left + title_width;
	ControlRef ref = make_static_text( win, r, title );
	
	r.left = r.right;
	r.right = bounds.right;
	slider.make( win, r, pointer_ticks, callback, data );
	
	return ref;
}

void Titled_Slider::make( WindowRef win, const Rect& bounds, const char* title,
		int title_width, int pointer_ticks, callback_t callback, void* data )
{
	text.reset( make_titled_slider( *this, win, bounds, title, title_width, pointer_ticks,
			callback, data ) );
}

void Titled_Slider::enable( bool b ) {
	text.enable( b );
	Slider_Control::enable( b );
}

void press_button( ControlRef c, int delay )
{
	Button_Pusher bp( c, delay );
}

static void redraw_control( ControlRef control )
{
	Draw1Control( control );
	
	WindowRef window = GetControlOwner( control );
	CGrafPtr port = GetWindowPort( window );
	
	if ( QDIsPortBuffered( port ) )
	{
		Rect r;
		GetControlBounds( control, &r );
		
		RgnHandle rgn = NewRgn();
		if ( rgn )
			RectRgn( rgn, &r );
		
		QDFlushPortBuffer( port, rgn ); // if NULL, flushes entire port
			
		if ( rgn )
			DisposeRgn( rgn );
	}
}

Button_Pusher::Button_Pusher( ControlRef control, int delay ) :
	button( IsControlActive( control ) ? control : NULL ),
	end_time( TickCount() + delay )
{
	if ( button ) {
		HiliteControl( button, kControlIconPart );
		redraw_control( button );
	}
}

Button_Pusher::~Button_Pusher()
{
	if ( button && IsControlActive( button ) ) {
		int remain = end_time - TickCount();
		if ( remain > 0 )
			Delay( remain, &end_time );
		HiliteControl( button, kControlNoPart );
		redraw_control( button );
	}
}

void enable_control( ControlRef control, bool b )
{
	if ( IsControlActive( control ) != b )
	{
		HiliteControl( control, kControlNoPart ); // in case it was hilited
		if ( b )
			ActivateControl( control );
		else
			DeactivateControl( control );
	}
}

void make_default_button( ControlRef button, bool b )
{
	Boolean bb = b;
	debug_if_error( SetControlData( button, kControlButtonPart,
			kControlPushButtonDefaultTag, sizeof b, &b ) );
}

ControlRef get_root_control( WindowRef window )
{
	ControlRef result = NULL;
	return (noErr == GetRootControl( window, &result )) ? result : NULL;
}

ControlRef make_static_icon( WindowRef w, const Rect& r, int res_id )
{
	ControlButtonContentInfo info;
	info.contentType = kControlContentCIconRes;
	info.u.resID = res_id;
	ControlRef control;
	throw_if_error( CreateIconControl( w, &r, &info, true, &control ) );
	return control;
}

void Static_Text::enable_truncation( bool b )
{
	// to do: has no effect on OS X
	TruncCode trunc = truncEnd;
	debug_if_error( SetControlData( control, kControlEditTextPart,
			kControlStaticTextTruncTag, sizeof trunc, &trunc ) );
}

void set_control_help( ControlRef control, const char* str )
{
	static const HMHelpContentRec init = { kMacHelpVersion, { 0, 0, 0, 0 },
			kHMOutsideBottomCenterAligned, kHMPascalStrContent };
	HMHelpContentRec hc = init;
	c2pstrcpy( hc.content [0].u.tagString, str );
	HMSetControlHelpContent( control, &hc );
}

