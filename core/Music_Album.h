
// Game_Music_Box 0.5.2. Copyright (C) 2005 Shay Green. GNU LGPL license.

#ifndef MUSIC_ALBUM_H
#define MUSIC_ALBUM_H

#include "common.h"
#include "File_Archive.h"
class Music_Emu;
class Multi_Buffer;

class Music_Album;

int album_track_count( const FSRef&, OSType music_type, const HFSUniStr255& );

Music_Album* load_music_album( const FSRef& );

// more optimal
Music_Album* load_music_album( const FSRef&, OSType music_type, const HFSUniStr255& );

class Music_Album {
public:
	Music_Album();
	virtual ~Music_Album() { }
	
	// nsf_type, gbs_type, etc.
	long music_type() const;
	
	// Return closest sample rate to requested
	virtual long sample_rate( long requested ) const { return requested; }
	
	virtual Music_Emu* make_emu( long sample_rate, Multi_Buffer* = NULL ) = 0;
	
	// Load track into emulator and return remapped track. If 'already_loaded' is
	// true, skip loading.
	int load_track( Music_Emu&, int track, bool already_loaded = false );
	int track_count();
	
	enum { max_field = 128 };
	struct info_t
	{
		const char** channel_names;
		const char* system;
		bool classic_emu; // true if emulator is derived from Classic_Emu
		int track_count;
		
		char filename [max_field];
		char game [max_field];
		char author [max_field];
		char copyright [max_field];
		
		char song [max_field];
		int duration;
		bool is_song; // false if really short or tagged as sound effect etc.
	};
	const info_t& info() const;
	
	// Closes archive and frees memory, but doesn't affect ability to load tracks.
	void uncache();
	
protected:
	info_t info_;
	
	virtual bool is_file_supported( const char* filename ) const = 0;
	
	// New track needs to be loaded. If archive is not NULL, new file must
	// be loaded first. Return (possibly remapped) track number to be started
	// in emulator.
	virtual int load_track_( Music_Emu&, int track, File_Archive* ) = 0;
	
	// Call when starting a new track and track info changes, but no new file was loaded
	void info_changed();
	
	static void set_field( char*, int, int ) { }
	static void set_field( char* field, const char* in, int size );
	
	void set_track_count( int );
	
private:
	BOOST::scoped_ptr<File_Archive> archive_;
	FSRef archive_path;
	bool use_parent;
	long music_type_;
	int current_track;
	int next_index;
	int track_count_; // 0 until read
	int file_count;
	
	void seek_archive( int track );
	friend Music_Album* load_music_album( const FSRef&, OSType, const HFSUniStr255& );
};

// End of public interface
int nsf_track_count( File_Archive& );
int nsfe_track_count( File_Archive& );
int gbs_track_count( File_Archive& );

Music_Album* new_nsf_album();
Music_Album* new_nsfe_album();
Music_Album* new_gbs_album();
Music_Album* new_vgm_album();
Music_Album* new_gym_album();
Music_Album* new_spc_album();
Music_Album* new_spcp_album( const FSRef& );

inline const Music_Album::info_t& Music_Album::info() const {
	return info_;
}
inline long Music_Album::music_type() const {
	return music_type_;
}
#endif

