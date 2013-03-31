
/* Game_Music_Box 0.5.2. Copyright (C) 2005 Shay Green. GNU LGPL license. */

#ifdef __cplusplus
	
	// Uncomment to use precompiled header
	#include "ram:prefix"
	
	#define HAVE_CONFIG_H 1
	
	#define TARGET_API_MAC_CARBON 1
	
	#include "config.h"
	
#endif

/* Generate zlib CRC32 table at run-time (saves about 6K) */
#define DYNAMIC_CRC_TABLE

