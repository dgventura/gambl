
// Drag Manager wrapper and utilities

// Game_Music_Box 0.5.2. Copyright (C) 2005 Shay Green. GNU LGPL license.

#ifdef GMB_COMPILE_GUI

#ifndef DRAG_HANDLER_H
#define DRAG_HANDLER_H

#include "common.h"
//#include <Drag.h>

class Drag_Handler : noncopyable {
public:
	Drag_Handler();
	~Drag_Handler();
	
protected:
	void init( WindowRef );
	
	// Calls handle_drag() for each item dropped. Called internally when
	// a drop occurs. Allows pre- and post- processing for a drop.
	virtual void items_dropped( DragReference );
	
private:
	virtual bool handle_drag( const FSRef&, OSType type, bool take_action );
	
	// Could be made virtual in the future
	bool process_item( DragReference, int index, bool take_action );
	void highlight_window( bool );
	bool is_in_drop_area( const Point& ) const;
	
	bool is_in_drop_area_();
	
	WindowRef window_;
	DragReference cur_drag;
	bool accepted;
	bool highlighted;
	
	static DragTrackingHandlerUPP track_handler_upp;
	static DragReceiveHandlerUPP receive_handler_upp;
	
	static pascal OSErr receive_handler( WindowRef, void*, DragReference );
	static pascal OSErr track_handler( DragTrackingMessage, WindowRef, void*, DragReference );
	OSErr track_handler_( DragTrackingMessage, DragReference );
};

ItemReference GetDragItemReferenceNumberChk( DragReference, int index );
int CountDragItemFlavorsChk( DragReference, ItemReference );
FlavorType GetFlavorTypeChk( DragReference, ItemReference, int index );
bool IsFlavorPresent( DragReference, ItemReference, FlavorType );
HFSFlavor GetFlavorData_HFSChk( DragReference, ItemReference );
int CountDragItemsChk( DragReference );
void ShowDragHiliteChk( DragReference, RgnHandle, bool inside );
void HideDragHiliteChk( DragReference );

#endif

#endif // #ifdef GMB_COMPILE_GUI
