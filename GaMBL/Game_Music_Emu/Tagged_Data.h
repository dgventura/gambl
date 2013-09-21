
// Simple extensible tagged data items, with nesting support

// Game_Music_Box 0.5.2. Copyright (C) 2005 Shay Green. BSD license.

#ifndef TAGGED_DATA_H
#define TAGGED_DATA_H

#include <stdio.h>

typedef long data_tag_t;

class Tagged_Data {
public:
	typedef long tag_t;
	
	// Nest data in parent container.
	Tagged_Data( Tagged_Data& parent, data_tag_t );
	
	// True if this is a read session, false if a write session.
	int reading() const;
	
	// True if reading and container doesn't exist in parent.
	int not_found() const;
	
	// Acessors take and return a value. If reading and the tagged
	// data exists, they read and return the data; if it doesn't exist,
	// they return the value passed. If this is a write session, the
	// value is written and also returned.
	
	// Access 8-bit integer with given tag and return value read.
	int reflect_int8( data_tag_t, int );
	
	// Access 16-bit integer with given tag and return value read.
	int reflect_int16( data_tag_t, int );
	
	// Access 32-bit integer with given tag and return value read.
	long reflect_int32( data_tag_t, long );
	
	~Tagged_Data();
	
private:
	char* pos;
	char* data_out;
	Tagged_Data* parent;
	
	Tagged_Data();
	long size();
	long reflect_int( data_tag_t, long, int field_size );
	
	friend class Tagged_Reader;
	friend class Tagged_Writer;
};

// Top-level container for writing new data to file
class Tagged_Writer : public Tagged_Data {
	enum { buf_size = 8192 }; // to do: auto-expanding buffer!
	char* buf;
public:
	Tagged_Writer();
	~Tagged_Writer();
	
	// Write data to file with specified tag. NULL on success, otherwise
	// error string.
	const char* write( FILE*, data_tag_t );
};

// Top-level container for reading data from file
class Tagged_Reader : public Tagged_Data {
	char* buf;
public:
	Tagged_Reader();
	~Tagged_Reader();
	
	// Read data from file if header with specified tag exists, otherwise
	// leave file at current position. NULL on success, otherwise error
	// string.
	const char* read( FILE*, data_tag_t );
	
	// Print entire contents to standard output.
	void debug_print();
};

template<class T>
inline void reflect_int8( Tagged_Data& data, data_tag_t tag, T* t ) {
	*t = data.reflect_int8( tag, *t );
}

template<class T>
inline void reflect_int16( Tagged_Data& data, data_tag_t tag, T* t ) {
	*t = data.reflect_int16( tag, *t );
}

template<class T>
inline void reflect_int32( Tagged_Data& data, data_tag_t tag, T* t ) {
	*t = data.reflect_int32( tag, *t );
}

	inline int Tagged_Data::reading() const {
		return data_out == 0;
	}

	inline int Tagged_Data::not_found() const {
		return pos == 0;
	}

#endif

