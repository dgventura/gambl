
// Game_Music_Box 0.5.2. http://www.slack.net/~ant/game-music-box

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

// Event_Loop_Timer

Event_Loop_Timer::~Event_Loop_Timer() {
	remove();
}
	
void Event_Loop_Timer::install( double interval, double delay ) {
	remove();
//DEPRECATED	static EventLoopTimerUPP os_callback_upp = throw_if_null(
//DEPRECATED			NewEventLoopTimerUPP( os_callback ) );
//DEPRECATED	throw_if_error( InstallEventLoopTimer( GetMainEventLoop(),
//DEPRECATED			delay * kEventDurationSecond, interval * kEventDurationSecond,
//DEPRECATED			os_callback_upp, this, &timer ) );
}

void Event_Loop_Timer::one_shot( double delay ) {
	install( kEventDurationForever, delay );
}

void Event_Loop_Timer::remove() {
//DEPRECATED	if ( timer ) {
//DEPRECATED		debug_if_error( RemoveEventLoopTimer( timer ) );
//DEPRECATED		timer = NULL;
//DEPRECATED	}
}

void Event_Loop_Timer::set_next_time( double delay ) {
//DEPRECATED	throw_if_error( SetEventLoopTimerNextFireTime( timer, delay * kEventDurationSecond ) );
}

pascal void Event_Loop_Timer::os_callback( EventLoopTimerRef, void* data ) {
	Event_Loop_Timer* self = static_cast<Event_Loop_Timer*> (data);
	if ( self->callback )
		self->callback( self->callback_data );
}

// Deferred_Task

Deferred_Task::Deferred_Task( callback_t func, void* data )
{
	callback = func;
	callback_data = data;
	installed = false;
	
	task.qType = dtQType;
	task.dtFlags = 0;
	task.dtParam = reinterpret_cast<long> (this);
	static DeferredTaskUPP os_callback_upp =
			throw_if_null( NewDeferredTaskUPP( os_callback ) );
	task.dtAddr = os_callback_upp;
	task.dtReserved = 0;
}

Deferred_Task::~Deferred_Task()
{
	assert( !installed );
}

bool Deferred_Task::install()
{
	if ( installed )
		return true;
	
	installed = true;
	sync_memory();
    
    assert(0);
//DEPRECATED	if ( debug_if_error( DTInstall( &task ) ) ) {
//DEPRECATED		installed = false;
//DEPRECATED		return true;
//DEPRECATED	}
	
	return false;
}

pascal void Deferred_Task::os_callback( long param )
{
	Deferred_Task* dt = reinterpret_cast<Deferred_Task*> (param);
	dt->callback( dt->callback_data );
	dt->installed = false;
	sync_memory();
}

// Virtual_Memory_Holder

void Virtual_Memory_Holder::hold( void* p, std::size_t s ) {
	begin = p;
	size = s;
//DEPRECATED	debug_if_error( HoldMemory( begin, size ) );
}

Virtual_Memory_Holder::~Virtual_Memory_Holder() {
//DEPRECATED	if ( begin )
//DEPRECATED		debug_if_error( UnholdMemory( begin, size ) );
}

// sync_memory

static void sync_memory_()
{
	// probably an overkill, but isn't used in a way that's performance-critical
//DEPRECATED	__sync();
//DEPRECATED	__isync();
}

// ensure compiler doesn't try to optimize this at all
static void (*sync_memory_ptr)() = sync_memory_;

void sync_memory()
{
	// to do: use MPProcessors() and sync only when there is more than one
	// (0.1% CPU overhead on 400 MHz G3)
	// if ( MPIsFullyInitialized() && MPProcessors() != 1 )
	sync_memory_ptr();
}

