
// Game_Music_Box 0.5.2. Copyright (C) 2005 Shay Green. GNU LGPL license.

#ifdef COMPILE_GMB_GUI

#ifndef SCOPE_WINDOW_H
#define SCOPE_WINDOW_H

#include "common.h"
#include "thread_util.h"
#include "Carbon_Window.h"

class Scope_Renderer {
public:
	Scope_Renderer();
	~Scope_Renderer();
	
	void resize( int width, int height );
	void clear();
	void render( const short* in, int in_step, short* min_out = NULL, short* max_out = NULL );
	void draw( const point& top_left, int min = 0, int max = 0 );
	
private:
	typedef unsigned char byte;
	runtime_array<byte> old_buf;
	GWorldPtr gworld;
	CTabHandle color_table;
	rect bounds;
	byte* pixels;
	int row_bytes;
	int sample_shift;
	int old_max;
	int old_min;
};

class Scope_Window : public Carbon_Window {
public:
	Scope_Window();
	virtual ~Scope_Window();
	
	void clear( bool flatline = false );
	void show( bool = true );
	void set_buffer( const short* );
	
protected:
	virtual const short* get_buffer( bool update_only );
private:
	Event_Loop_Timer timer;
	Scope_Renderer scope;
	const short* buffer;
	
	void update_( bool );
	static void update( void* );
	void draw_content();
	bool handle_command( const HICommandExtended& );
	void redraw( int top, int bottom );
	void set_zoom( bool );
	void draw( int min, int max );
};

inline void Scope_Window::set_buffer( const short* p ) {
	buffer = p;
}

#endif

#endif // COMPILE_GMB_GUI