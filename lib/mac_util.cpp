
// Game_Music_Box 0.5.2. http://www.slack.net/~ant/game-music-box

#ifdef GMB_COMPILE_GUI

#include "mac_util.h"

#include <exception>
#include <cstdio>
//#include <Gestalt.h>
//#include <Sound.h>
//#include <Dialogs.h>
//#include <Notification.h>
//#include <InternetConfig.h>
//#include <LaunchServices.h>
//#include <MacHelp.h>
#include "file_util.h"

/* This module copyright (C) 2005 Shay Green. This module is free software;
you can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
module is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details. You should have received a copy of the GNU Lesser General
Public License along with this module; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA */

#include "source_begin.h"

static void AEInstallEventHandlerChk( AEEventClass cl, AEEventID id, AEEventHandlerUPP upp,
		long ref, Boolean sys ) {
	 throw_if_error( AEInstallEventHandler( cl, id, upp, ref, sys ) );
}

void install_ae_handler( AEEventID id, AEEventHandlerProcPtr proc, void* user_data ) {
	AEInstallEventHandlerChk( kCoreEventClass, id,
			throw_if_null( NewAEEventHandlerUPP( proc ) ),
			reinterpret_cast<long> (user_data), false );
}

bool is_aqua_ui() {
	return false;
}

static void post_notification( const void* text, int len )
{
	static NMRec nm;
	
	static Str255 nstr;
	
	// don't change old str if notification was already installed
	if ( !nm.qType || NMRemove( &nm ) != 0 ) {
		nstr [0] = len;
		std::memcpy( &nstr [1], text, nstr [0] );
	}
	
	// (re)install notification
	nm.qType = nmType;
	nm.nmFlags = 0;
	nm.nmReserved = 0;
	nm.nmMark = 1;
	nm.nmIcon = NULL;
	nm.nmSound = (Handle) -1; // play alert sound
	nm.nmStr = nstr;
	nm.nmResp = (NMUPP) -1; // automatically remove
	NMInstall( &nm );
}

static void post_notification( const char* str ) {
	post_notification( str, std::strlen( str ) );
}

bool is_front_app()
{
	ProcessSerialNumber front;
	ProcessSerialNumber current = { 0, kCurrentProcess };
	
	GetFrontProcess( &front );
	Boolean result = false;
	return !SameProcess( &current, &front, &result ) && result;
}

void app_to_front()
{
	ProcessSerialNumber current = { 0, kCurrentProcess };
	debug_if_error( SetFrontProcess( &current ) );
}

void standard_alert( AlertType type, const char* str )
{
	Str255 pstr;
	c2pstrcpy( pstr, str );
	SInt16 item;
	debug_if_error( StandardAlert( type, pstr, "\p", NULL, &item ) );
}

void report_error( const char* str, bool deferred )
{
	InitCursor();
	if ( deferred && !is_front_app() )
	{
		post_notification( str );
	}
	else
	{
		SysBeep( 20 );
		standard_alert( kAlertStopAlert, str );
	}
}

long report_exception( bool deferred )
{
	char str [exception_str_max];
	str [0] = 0;
	if ( !exception_to_str( str ) )
		std::strcpy( str, "An error occurred" );
	long result = exception_to_code();
	report_error( str, deferred );
	return result;
}

rect::rect( int l, int t, int r, int b ) {
	top = t;
	left = l;
	bottom = b;
	right = r;
}

EventHandlerUPP Event_Handler::event_handler;

Event_Handler::Event_Handler()
{
	app_event_handler = NULL;
	if ( !event_handler )
		throw_if_null( event_handler = NewEventHandlerUPP( handle_event_ ) );
}

Event_Handler::~Event_Handler()
{
	if ( app_event_handler )
		debug_if_error( RemoveEventHandler( app_event_handler ) );
}

void Event_Handler::receive_app_events()
{
	static const EventTypeSpec type = { kEventClassCommand, kEventCommandProcess };
	debug_if_error( InstallApplicationEventHandler( event_handler, 1, &type,
			event_handler_data(), &app_event_handler ) );
}


bool Event_Handler::handle_command( const HICommandExtended& ) {
	return false;
}

bool Event_Handler::handle_event( EventRef ) {
	return false;
}

pascal OSStatus Event_Handler::handle_event_( EventHandlerCallRef next_handler, EventRef event,
		void* data )
{
	try
	{
		HICommandExtended cmd;
		switch ( GetEventClass( event ) )
		{
			case kEventClassCommand:
			{
				debug_if_error( GetEventParameter( event, kEventParamDirectObject,
						typeHICommand, NULL, sizeof cmd, NULL, &cmd ) );
				if ( !static_cast<Event_Handler*> (data)->handle_command( cmd ) )
					return eventNotHandledErr;
				break;
			}
			
			default:
				if ( !static_cast<Event_Handler*> (data)->handle_event( event ) )
					return CallNextEventHandler( next_handler, event );
				break;
		}
	}
	catch ( ... ) {
		return report_exception();
	}
	
	return noErr;
}

Nav_Reply::Nav_Reply( NavDialogRef dialog )
{
	size_ = -1;
	version = kNavReplyRecordVersion;
	OSStatus result = NavDialogGetReply( dialog, this );
	if ( result != userCanceledErr )
		throw_if_error( result );
}

int Nav_Reply::size() const
{
	if ( size_ < 0 )
		throw_if_error( AECountItems( &selection, &const_cast<Nav_Reply*> (this)->size_ ) );
	return size_;
}

FSRef Nav_Reply::operator [] ( int i ) const {
	FSRef result;
	AEKeyword keyword;
	DescType type;
	Size size;
	throw_if_error( AEGetNthPtr( &selection, i + 1, typeFSRef, &keyword, &type,
			&result, sizeof result, &size ) );
	return result;
}

Nav_Reply::~Nav_Reply() {
	NavDisposeReply( this );
}

void send_quit_event()
{
	bool sent = false;
	EventRef event = NULL;
	if ( !debug_if_error( MacCreateEvent( NULL, kEventClassCommand, kEventCommandProcess,
			GetCurrentEventTime(), kEventAttributeUserEvent, &event ) ) )
	{
		HICommandExtended cmd;
		cmd.attributes = 0;
		cmd.commandID = kHICommandQuit;
		if ( !debug_if_error( SetEventParameter( event, kEventParamDirectObject,
						typeHICommand, sizeof cmd, &cmd ) ) )
		{
			if ( !debug_if_error( SendEventToApplication( event ) ) )
				sent = true;
		}
		ReleaseEvent( event );
	}
	
	if ( !sent )
		QuitApplicationEventLoop();
}

static OSType get_app_signature_() {
	ProcessSerialNumber psn = { 0, kCurrentProcess };
	ProcessInfoRec info;
	info.processInfoLength = sizeof info;
	info.processName = NULL;
	info.processAppSpec = NULL;
	throw_if_error( GetProcessInformation( &psn, &info ) );
	return info.processSignature;
}

OSType get_app_signature() {
	static OSType s = get_app_signature_();
	return s;
}

Ic_Session::Ic_Session() {
	throw_if_error( ICStart( &ic, get_app_signature() ) );
}

Ic_Session::~Ic_Session() {
	debug_if_error( ICStop( ic ) );
}

void launch_url( const char* str )
{
	try {
		Ic_Session ic;
		long begin = 0;
		long end = std::strlen( str );
		ICLaunchURL( ic, "\p", str, end, &begin, &end );
	}
	catch ( ... ) {
		check( false );
	}
}

bool is_os_x()
{
	static long result;
	if ( !result )
		debug_if_error( Gestalt( gestaltSystemVersion, &result ) );
	return result >= 0x1000;
}

static bool GetNextProcessChk( ProcessSerialNumber* sn )
{
	OSErr const err = GetNextProcess( sn );
	if ( err == procNotFound )
		return false;
	throw_if_error( err );
	return true;
}

static void GetProcessInformationChk( ProcessSerialNumber const& sn, ProcessInfoRec* info ) {
	throw_if_error( GetProcessInformation( &sn, info ) );
}

static bool FindProcessBySignature( OSType creator, ProcessSerialNumber* psn_out, OSType type )
{
	ProcessSerialNumber sn;
	sn.highLongOfPSN = kNoProcess;
	sn.lowLongOfPSN = kNoProcess;
	
	ProcessInfoRec info;
	info.processInfoLength = sizeof info;
	info.processName = NULL;
	info.processAppSpec = NULL;
	
	while ( GetNextProcessChk( &sn ) )
	{
		GetProcessInformationChk( sn, &info );
		
		if ( info.processSignature == creator && (!type || info.processType == type) ) {
			if ( psn_out )
				*psn_out = sn;
			return true;
		}
	}
	return false;
}

class AEAutoDisposeDesc {
	AEDesc desc;
public:
	AEAutoDisposeDesc() {
		desc.dataHandle = NULL;
	}
	~AEAutoDisposeDesc() {
		dispose();
	}
	
	void dispose();
	
	operator const AEDesc* () const {
		return &desc;
	}
	
	AEDesc* operator & () {
		return &desc;
	}
};

void AEAutoDisposeDesc::dispose()
{
	if ( desc.dataHandle ) {
		AEDisposeDesc( &desc );
		desc.dataHandle = NULL;
	}
}

class Handle_Disposer {
	void* handle;
public:
	Handle_Disposer( void* h ) : handle( h ) { }
	~Handle_Disposer();
};

Handle_Disposer::~Handle_Disposer()
{
	if ( handle )
		DisposeHandle( static_cast<Handle> (handle) );
}

static void make_ae_alias( const FSRef& path, AEDesc* out )
{
	AliasHandle alias = NULL;
	throw_if_error( FSNewAlias( NULL, &path, &alias ) );
	Handle_Disposer disposer( alias );
	HLock( (Handle) alias );
	throw_if_error( AECreateDesc( typeAlias, *alias, GetHandleSize( (Handle) alias ), out ) );
}

static void AEPutParamDescChk( AppleEvent* event, AEKeyword keyword, AEDesc const* desc ) {
	throw_if_error( AEPutParamDesc( event, keyword, desc ) );
}

static void AESendChk( AppleEvent const* event, AppleEvent* reply, AESendMode mode,
		AESendPriority pri = kAENormalPriority, long timeout = kAEDefaultTimeout,
		AEIdleUPP idle = NULL, AEFilterUPP filter = NULL ) {
	throw_if_error( AESend( event, reply, mode, pri, timeout, idle, filter ) );
}

void launch_file( const FSRef& path )
{
	if ( LSOpenFSRef ) {
		debug_if_error( LSOpenFSRef( &path, NULL ) );
	}
	else {
		OSType const aeSelectionKeyword = 'fsel';
		OSType const kAEOpenSelection = 'sope';
		OSType const finder_type = 'FNDR';
		
		ProcessSerialNumber process;
		FindProcessBySignature( 'MACS', &process, finder_type );
		
		AEAutoDisposeDesc psn_desc;
		throw_if_error( AECreateDesc( typeProcessSerialNumber, &process, sizeof process, &psn_desc ) );
	
		AEAutoDisposeDesc event;
		throw_if_error( AECreateAppleEvent( finder_type, kAEOpenSelection, psn_desc,
				kAutoGenerateReturnID, kAnyTransactionID, &event ) );
		
		// to do: verify that this simpler method is correct
		{
			AEAutoDisposeDesc alias;
			make_ae_alias( path, &alias );
			
			AEPutParamDescChk( &event, keyDirectObject, alias );
		}
		
		/*
		{
			AEAutoDisposeDesc parent_alias;
			make_ae_alias( get_parent( path ), &parent_alias );
			
			AEPutParamDescChk( &event, keyDirectObject, parent_alias );
		}
		
		if ( false )
		
		//if ( false )
		{
			AEAutoDisposeDesc list;
			throw_if_error( AECreateList( NULL, 0, false, &list ) );
			
			AEAutoDisposeDesc alias;
			make_ae_alias( path, &alias );
			
			throw_if_error( AEPutDesc( &list, 0, alias ) );
			
			AEPutParamDescChk( &event, aeSelectionKeyword, list );
		}
		*/
		
		AEAutoDisposeDesc reply;
		AESendChk( event, &reply, kAENoReply | kAEAlwaysInteract | kAECanSwitchLayer );
		
		if ( is_dir( path ) )
			debug_if_error( SetFrontProcess( &process ) );
	}
}

void add_help_item( const char* title, long command, char key )
{
	MenuRef menu = NULL;
	debug_if_error( HMGetHelpMenu( &menu, NULL ) );
	if ( menu )
	{
		if ( !title ) {
			MacAppendMenu( menu, "\p-" );
		}
		else {
			Str255 str;
			c2pstrcpy( str, title );
			AppendMenuItemText( menu, str );
			int index = CountMenuItems( menu );
			SetMenuItemCommandID( menu, index, command );
			if ( key )
				SetMenuItemCommandKey( menu, index, false, key );
		}
	}
}

void open_help_document( const char* name )
{
	HFSUniStr255 filename;
	str_to_filename( name, filename );
	FSRef ref;
	if ( FSMakeFSRefExists( get_parent( get_bundle_fsref() ), filename, &ref ) )
		launch_file( ref );
}

void flush_window( WindowRef window, RgnHandle rgn )
{
	CGrafPtr port = GetWindowPort( window );
	if ( QDIsPortBuffered( port ) )
		QDFlushPortBuffer( port, rgn ); // if NULL, flushes entire port
}

int get_font_height( int id, int size, int style )
{
	FontInfo info;
	if ( !debug_if_error( FetchFontInfo( id, size, style, &info ) ) )
		return info.ascent + info.descent + info.leading;
	return size + size / 3;
}

// having empty event handler allows nav dialog to be movable and resizeable
static pascal void event_handler( NavEventCallbackMessage, NavCBRecPtr, NavCallBackUserData ) {
}

Nav_Options::Nav_Options( const char* client_name ) {
	version = kNavDialogCreationOptionsVersion;
	throw_if_error( NavGetDefaultDialogCreationOptions( this ) );
	clientName = __CFStringMakeConstantString( client_name );
	modality = kWindowModalityAppModal;
}

NavEventUPP default_nav_handler() {
	static NavEventUPP upp = NewNavEventUPP( event_handler );
	return upp;
}

#endif
