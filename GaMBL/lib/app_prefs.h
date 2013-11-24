
// CFPreferences utilities for apps

// Game_Music_Box 0.5.2. Copyright (C) 2005 Shay Green. GNU LGPL license.

#ifndef APP_PREFS_H
#define APP_PREFS_H

#include "common.h"
//#include <CFNumber.h>

void update_app_prefs();

bool reflect_app_pref( bool set, void* value, const char* name_, CFNumberType );

template<class T>
bool reflect_pref_int( bool set, T* value, const char* name )
{
	int temp = *value;
	bool result = reflect_app_pref( set, &temp, name, kCFNumberIntType );
	*value = temp;
	return result;
}

template<class T>
bool reflect_pref_float( bool set, T* value, const char* name )
{
	float temp = *value;
	bool result = reflect_app_pref( set, &temp, name, kCFNumberFloatType );
	*value = temp;
	return result;
}

#endif

