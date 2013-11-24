
// Game_Music_Box 0.5.2. http://www.slack.net/~ant/game-music-box

#include "app_prefs.h"

//#include <CFPreferences.h>

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

// Boolean type is a bit different to handle. Use kCFBooleanTrue and kCFBooleanFalse
// as CFNumberRefs for those values.

bool reflect_app_pref( bool set, void* value, const char* name_, CFNumberType type )
{
	CFStringRef name = __CFStringMakeConstantString( name_ );
	bool result = false;
	if ( set ) {
		CFNumberRef ref = CFNumberCreate( NULL, type, value );
		CFPreferencesSetAppValue( name, ref, kCFPreferencesCurrentApplication );
		CFRelease( ref );
	}
	else {
		CFNumberRef ref = (CFNumberRef)
				CFPreferencesCopyAppValue( name, kCFPreferencesCurrentApplication );
		if ( ref ) {
			result = true;
			CFNumberGetValue( ref, type, value );
		}
	}
	
	return result;
}

void update_app_prefs()
{
	debug_if_error( !CFPreferencesAppSynchronize( kCFPreferencesCurrentApplication ) );
}

