
// Actions to perform on game music files and folders

// Game_Music_Box 0.5.2. http://www.slack.net/~ant/game-music-box

#ifndef MUSIC_ACTIONS_H
#define MUSIC_ACTIONS_H

#include "common.h"

#include "music_util.h"
class Cat_Info;

// Fix file type of a single file. Return error string or NULL if successful.
const char* fix_music_file_type( const Cat_Info&, bool change_type = true );

struct Action_Hooks
{
	virtual bool give_time() = 0;
	virtual bool is_scanning() const = 0;
	
	// True if action should be taken on item
	virtual bool advance( const FSRef&, int count = 1 ) = 0;
	virtual void log_error( const FSRef&, const char* ) = 0;
	virtual void log_exception( const FSRef& ) = 0;
};

void associate_music( const FSRef&, Action_Hooks&, bool hide_ext );

void retitle_music( const FSRef&, Action_Hooks& );

void compress_music( const FSRef&, Action_Hooks&, bool pack_spcs );

void expand_music( const FSRef&, Action_Hooks& );

void check_music_files( const FSRef&, Action_Hooks& );

#endif

