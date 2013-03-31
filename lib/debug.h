
#ifndef DEBUG_H
#define DEBUG_H

struct Debug_Printf {
	const char* file;
	int line;
	Debug_Printf( const char* f, int l ) : file( f ), line( l ) { }
	void operator () ( const char* fmt, ... );
};

#endif

