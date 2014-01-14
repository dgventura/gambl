
// Favorites menu

// Game_Music_Box 0.5.2. Copyright (C) 2005 Shay Green. GNU LGPL license.

#ifndef FAVORITES_H
#define FAVORITES_H

#include "common.h"
#include "wave_export.h"
class Player_Window;

extern const char* system_names [5];

void play_classic_favorites( Player_Window& );
void play_favorites( Player_Window&, const char* name );
void open_favorites();
void add_favorite( const track_ref_t&, shared_ptr< Music_Album > pAlbum );
const std::wstring& favorites_dir();

#endif

