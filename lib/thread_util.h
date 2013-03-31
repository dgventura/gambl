
// Game_Music_Box 0.5.2. Copyright (C) 2005 Shay Green. GNU LGPL license.

#ifndef THREAD_UTIL_H
#define THREAD_UTIL_H

#include "common.h"

class Event_Loop_Timer : noncopyable {
public:
	Event_Loop_Timer();
	~Event_Loop_Timer();
	
	void set_callback( void (*func)( void* data ), void* data );
	
	void install( double interval, double delay = 0 );
	void one_shot( double delay );
	bool installed() const { return timer != NULL; }
	void set_next_time( double delay );
	void remove();
	
private:
	EventLoopTimerRef timer;
	void (*callback)( void* );
	void* callback_data;
	static pascal void os_callback( EventLoopTimerRef, void* );
};

class Deferred_Task : noncopyable {
public:
	
	typedef void (*callback_t)( void* data );
	
	Deferred_Task( callback_t, void* data );
	~Deferred_Task();
	
	// Install task or return true if it was already installed or install failed
	bool install();
	
private:
	DeferredTask task;
	callback_t volatile callback;
	void* volatile callback_data;
	int volatile installed;
	static pascal void os_callback( long );
};

// Virtual memory holder
class Virtual_Memory_Holder {
	void* begin;
	std::size_t size;
public:
	Virtual_Memory_Holder() : begin( NULL ) { }
	void hold( void* begin, std::size_t count );
	void hold( void* begin, const void* end );
	~Virtual_Memory_Holder();
};

// Ensure memory is synchronized with other processors. Use before reading
// and after writing to data shared between threads.
void sync_memory();


// End of public interface
inline Event_Loop_Timer::Event_Loop_Timer() : timer( NULL ), callback( NULL ) {
}
inline void Event_Loop_Timer::set_callback( void (*func)( void* data ), void* data ) {
	callback = func;
	callback_data = data;
}
inline void Virtual_Memory_Holder::hold( void* begin, const void* end ) {
	hold( begin, (char*) end - (char*) begin );
}

#endif

