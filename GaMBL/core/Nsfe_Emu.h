
// Nintendo Entertainment System (NES) NSFE-format game music file emulator

// Game_Music_Emu 0.2.6. Copyright (C) 2005 Shay Green. GNU LGPL license.

#ifndef NSFE_EMU_H
#define NSFE_EMU_H

#include "blargg_common.h"
#include "Nsf_Emu.h"

// to do: eliminate dependence on bloated std vector
#include <vector>

class Nsfe_Emu : public Nsf_Emu {
public:
	// See Nsf_Emu.h for further information
	
	Nsfe_Emu( double gain = 1.4 );
	~Nsfe_Emu();
	
	struct header_t
	{
		char tag [4]; // 'N', 'S', 'F', 'E'
	};
	BOOST_STATIC_ASSERT( sizeof (header_t) == 4 );
	
	// Load NSFE
	blargg_err_t load( Emu_Reader& );
	
	// Load NSFE using already-loaded header and remaining data
	blargg_err_t load( header_t const&, Emu_Reader& );
	
	// Information about current file
	struct info_t : Nsf_Emu::header_t
	{
		// These (longer) fields hide those in Nsf_Emu::header_t
		char game [256];
		char author [256];
		char copyright [256];
		char ripper [256];
	};
	const info_t& info() const { return info_; }
	
	// All track indicies are 0-based
	
	// Name of track [i], or "" if none available
	const char* track_name( int i ) const;
	
	// Duration of track [i] in milliseconds, negative if endless, or 0 if none available
	long track_time( int i ) const;
	
	// Optional playlist consisting of track indicies
	int playlist_size() const { return playlist.size(); }
	int playlist_entry( int i ) const { return playlist [i]; }
	
private:
	std::vector<char> track_name_data;
	std::vector<const char*> track_names;
	std::vector<unsigned char> playlist;
	std::vector<long> track_times;
	info_t info_;
};

#endif

