
// Game_Music_Box 0.5.2. Copyright (C) 2005 Shay Green. GNU LGPL license.

#ifndef mac_controls_H
#define mac_controls_H

#include "common.h"
//#include <Controls.h>
#include "mac_util.h"

extern const ControlFontStyleRec default_text_style;

// Row layout handler
class Row_Placer {
	rect m_bounds;
	int spacing;
public:
	Row_Placer( int width, point const& top_left = point( 4, 4 ) );
	
	// Set spacing between rectangles
	void set_spacing( int n ) { spacing = n; }
	
	// Place new row of given height. If 'width' is specified, left-justify.
	rect place( int height, int width = 0 );
	
	// Rectangle enclosing all rectangles placed so far.
	rect const& bounds() { return m_bounds; }
};

// Layout within a row. Items can be placed from left-to-right and/or right-to-left,
// then the center space filled.
class Row_Filler {
	rect m_bounds;
	int spacing;
	rect place( int height );
public:
	Row_Filler( rect const& bounds, int spacing = 4 );
	rect place_left( int width, int height = 0 );
	rect place_right( int width, int height = 0 );
	
	// Rectangle of remaining space in the middle of the row.
	rect remain( int height = 0 );
};

class Control : noncopyable {
protected:
	ControlRef control;
public:
	Control() : control( NULL ) { }
	
	void reset( ControlRef r ) { control = r; }
	operator ControlRef () const { return control; }
	
	void enable( bool );
	void show( bool );
	void set_command( long );
	void set_title( const char* );
};

class Value_Control : public Control {
public:
	long value() const;
	void set_value( long );
	
	void make_checkbox( WindowRef, const Rect&, const char* title, long command,
			bool auto_toggle = true );
	void make_radio( WindowRef, const Rect&, const char* title, long command );
};

class Static_Text : public Control {
public:
	void make( WindowRef, const Rect&, const ControlFontStyleRec& = default_text_style );
	
	void enable_truncation( bool = true );
	void set_text( const char* );
	void set_text( const char*, int len );
};

class Slider_Control : public Value_Control {
public:
	typedef void (*callback_t)( void* data, Slider_Control*, int part );
	
	// If 'pointer_ticks' is negative, arrow points up, otherwise down
	void make( WindowRef, const Rect&, int pointer_ticks = 0, callback_t = NULL,
			void* data = NULL );
	
	double fraction() const;
	void set_fraction( double );
	
private:
	callback_t callback;
	void* callback_data;
	static pascal void handle_slider( ControlRef, ControlPartCode );
};

// Slider with caption to left. Enables and disables caption along with slider.
class Titled_Slider : public Slider_Control {
public:
	void make( WindowRef, const Rect&, const char* title, int title_width,
			int pointer_ticks = 0, callback_t = NULL, void* data = NULL );
	void enable( bool );
private:
	Static_Text text;
};

// Pushes button in constructor and releases button in destructor after sufficient
// time has passed. Allows useful work to be done while button is being pressed,
// rather than just waiting for the delay.
class Button_Pusher {
	ControlRef button;
	unsigned long end_time;
public:
	// Keep 'button' pressed for at least 'delay/60' seconds.
	Button_Pusher( ControlRef, int delay = 10 );
	~Button_Pusher();
};

void set_control_help( ControlRef, const char* );
void set_control_title( ControlRef, const char* );
void enable_control( ControlRef, bool );
void press_button( ControlRef, int delay = 10 );
void set_button_icon( ControlRef, int res_id );
void make_default_button( ControlRef, bool = true );
ControlRef get_root_control( WindowRef );

ControlRef make_static_text( WindowRef, const Rect&, const char* text = "",
		const ControlFontStyleRec& = default_text_style );
ControlRef make_push_button( WindowRef, const Rect&, const char* title, long command );
ControlRef make_cicon_button( WindowRef, const Rect&, int res_id, long command );
ControlRef make_static_icon( WindowRef, const Rect&, int res_id );
ControlRef make_titled_slider( Slider_Control&, WindowRef, const Rect&, const char* title,
		int title_width, int pointer_ticks = 0, Slider_Control::callback_t = NULL,
		void* data = NULL );

#endif

