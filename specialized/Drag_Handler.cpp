
// Game_Music_Box 0.5.2. http://www.slack.net/~ant/game-music-box

#ifdef GMB_COMPILE_GUI

#include "Drag_Handler.h"

#include "mac_util.h"

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

ItemReference GetDragItemReferenceNumberChk( DragReference drag, int index )
{
	ItemReference item_ref;
	throw_if_error( GetDragItemReferenceNumber( drag, index, &item_ref ) );
	return item_ref;
}

int CountDragItemFlavorsChk( DragReference drag, ItemReference ref )
{
	UInt16 count = 0;
	throw_if_error( CountDragItemFlavors( drag, ref, &count ) );
	return count;
}

FlavorType GetFlavorTypeChk( DragReference drag, ItemReference ref, int index )
{
	FlavorType type;
	throw_if_error( GetFlavorType( drag, ref, index, &type ) );
	return type;
}

bool IsFlavorPresent( DragReference drag, ItemReference ref, FlavorType type )
{
	for ( int i = CountDragItemFlavorsChk( drag, ref ) + 1; --i; )
		if ( GetFlavorTypeChk( drag, ref, i ) == type )
			return true;
	return false;
}

HFSFlavor GetFlavorData_HFSChk( DragReference drag, ItemReference ref )
{
	HFSFlavor result;
	Size size = sizeof result;
	throw_if_error( GetFlavorData( drag, ref, flavorTypeHFS, &result, &size, 0 ) );
	return result;
}

int CountDragItemsChk( DragReference drag )
{
	unsigned short num_items;
	throw_if_error( CountDragItems( drag, &num_items ) );
	return num_items;
}

void ShowDragHiliteChk( DragReference drag, RgnHandle rgn, bool inside ) {
	throw_if_error( ShowDragHilite( drag, rgn, inside ) );
}

void HideDragHiliteChk( DragReference drag ) {
	throw_if_error( HideDragHilite( drag ) );
}

bool Drag_Handler::is_in_drop_area_()
{
	Point mouse;
	GetDragMouse( cur_drag, &mouse, NULL );
	return is_in_drop_area( mouse );
}

DragTrackingHandlerUPP Drag_Handler::track_handler_upp;
DragReceiveHandlerUPP Drag_Handler::receive_handler_upp;

Drag_Handler::Drag_Handler() : window_( NULL ), cur_drag( NULL ) {
}

void Drag_Handler::init( WindowRef w )
{
	if ( !track_handler_upp )
		track_handler_upp = throw_if_null( NewDragTrackingHandlerUPP( track_handler ) );
	if ( !receive_handler_upp )
		receive_handler_upp = throw_if_null( NewDragReceiveHandlerUPP( receive_handler ) );

	throw_if_error( InstallTrackingHandler( track_handler_upp, w, this ) );
	throw_if_error( InstallReceiveHandler( receive_handler_upp, w, this ) );
	window_ = w;
}

Drag_Handler::~Drag_Handler()
{
	debug_if_error( RemoveTrackingHandler( track_handler_upp, window_ ) );
	debug_if_error( RemoveReceiveHandler( receive_handler_upp, window_ ) );
}

bool Drag_Handler::process_item( DragReference drag, int index, bool take_action )
{
	ItemReference ref = GetDragItemReferenceNumberChk( drag, index + 1 );
	if ( IsFlavorPresent( drag, ref, flavorTypeHFS ) ) {
		HFSFlavor currHFSFlavor = GetFlavorData_HFSChk( drag, ref );
		FSRef ref;
		throw_if_error( FSpMakeFSRef( &currHFSFlavor.fileSpec, &ref ) );
		return handle_drag( ref, currHFSFlavor.fileType, take_action );
	}
	
	return false;
}

static OSErr ShowDragHiliteBox( DragReference drag, Rect box )
{
	RgnHandle box_rgn = NewRgn();
	RgnHandle inset_rgn = NewRgn();
	
	OSErr err = memFullErr;
	if ( !box_rgn || !inset_rgn )
		goto exit;
	
	RectRgn( box_rgn, &box );
	InsetRect( &box, 3, 3 );
	RectRgn( inset_rgn, &box );
	DiffRgn( box_rgn, inset_rgn, box_rgn );
	
	err = ShowDragHilite( drag, box_rgn, true );
	
exit:
	if ( box_rgn )
		DisposeRgn( box_rgn );
	if ( inset_rgn )
		DisposeRgn( inset_rgn );
	
	return err;
}

void Drag_Handler::highlight_window( bool b )
{
	if ( b != highlighted ) {
		highlighted = b;
		
		// to do: is this needed?
		//Rect r;
		//ClipRect( GetWindowPortBounds( window_, &r ) );
		
		if ( highlighted ) {
			Rect r;
			GetWindowPortBounds( window_, &r );
			throw_if_error( ShowDragHiliteBox( cur_drag, r ) );
		}
		else {
			HideDragHiliteChk( cur_drag );
		}
	}
}

bool Drag_Handler::is_in_drop_area( const Point& pt ) const
{
	Rect r;
	GetWindowBounds( window_, kWindowGlobalPortRgn, &r );
	return PtInRect( pt, &r );
	//return PtInRgn( pt, reinterpret_ptr<WindowPeek> (window_)->contRgn );
}

bool Drag_Handler::handle_drag( const FSRef&, OSType type, bool take_action ) {
	return false;
}

pascal OSErr Drag_Handler::track_handler( DragTrackingMessage msg, WindowRef,
		void* data, DragReference drag )
{
	OSErr err = noErr;
	
	try {
		err = static_cast<Drag_Handler*> (data)->track_handler_( msg, drag );
	}
	catch ( ... ) {
		err = report_exception();
	}
	
	return err;
}

OSErr Drag_Handler::track_handler_( DragTrackingMessage msg, DragReference drag )
{
	switch ( msg )
	{
		// kDragTrackingEnterHandler is only sent the first time a particular
		// drag enters a window in application. Dragging to another window
		// never sends kDragTrackingEnterHandler to the new window's handler,
		// just kDragTrackingEnterWindow.
		case kDragTrackingEnterHandler:
		case kDragTrackingEnterWindow: 
		case kDragTrackingInWindow:
		case kDragTrackingLeaveWindow:
		{
			if ( !cur_drag )
			{
				cur_drag = drag;
				highlighted = false;
				accepted = false;
				try {
					for ( int i = CountDragItemsChk( drag ); i--; ) {
						if ( process_item( drag, i, false ) ) {
							accepted = true;
							break;
						}
					}
				}
				catch ( ... ) {
					// don't report error (it should occur again if user completes drop)
					accepted = true;
				}
			}
			
			if ( msg == kDragTrackingEnterWindow )
				break;
			
			if ( accepted )
				highlight_window( msg != kDragTrackingLeaveWindow && is_in_drop_area_() );
		}
		break;
		
		case kDragTrackingLeaveHandler:
			cur_drag = NULL;
			accepted = false;
			highlighted = false;
			break;
		
		default:
			return paramErr;
	}
	
	return accepted ? noErr : dragNotAcceptedErr;
}

void Drag_Handler::items_dropped( DragReference drag )
{
	int num_items = CountDragItemsChk( drag );
	for ( int i = 0; i < num_items; ++i )
		if ( process_item( drag, i, true ) )
			accepted = true;
}

pascal OSErr Drag_Handler::receive_handler( WindowRef window_, void* data,
		DragReference drag )
{
	Drag_Handler* self = static_cast<Drag_Handler*> (data);
	if ( self->accepted )
	{
		self->accepted = false;
		try {
			if ( self->is_in_drop_area_() ) {
				self->highlight_window( false );
				self->items_dropped( drag );
			}
		}
		catch ( ... ) {
			return report_exception( true );
		}
	}
	
	return self->accepted ? noErr : dragNotAcceptedErr;
}

#endif
