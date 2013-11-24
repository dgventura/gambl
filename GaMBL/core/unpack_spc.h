
/* Super Nintendo SPC music file unpacker */

/* Packed_Spc 0.1.2. Copyright (C) 2004 Shay Green. GNU LGPL license. */

#ifndef UNPACK_SPC_H
#define UNPACK_SPC_H

#ifdef __cplusplus
	extern "C" {
#endif

/* True only if SPC file is packed. */
int is_packed_spc( const void* spc, long size );

/* Get name of shared data file for packed SPC, or NULL if not a packed SPC file. */
const char* spc_shared_filename( const void* spc, long size );

/* Unpack SPC given its data and shared data file both deflated and read into memory.
   The output buffer must be at least 0x10180 bytes. *Out_size is set to the number
   of bytes actually filled. NULL on success, otherwise error string. */
const char* unpack_spc( const void* spc, long spc_size, const void* shared, long shared_size,
		void* out, long* out_size );

/* Read a packed SPC given the ungzipped spc data and a path to the shared data
   file. The output buffer must be at least 0x10180 bytes. *Out_size is set to the
   number of bytes actually filled. NULL on success, otherwise error string. */
const char* read_packed_spc( const void* spc, long spc_size, const char* shared_path,
		void* out, long* out_size );

/* end of public interface */

struct packed_spc_t {
	char            header [0x100]; /* header [0xD8] & 0x01 set if packed */
	char            format;         /* 'P' */
	char            flags;          /* currently unused */
	unsigned char   checksum [2];
	char            unused [60];
	char            shared_filename [64];
	unsigned char   cmp [0x10000 - 128];
	unsigned char   dsp [0x80];     /* DSP registers and any data following them */
};

enum packed_spc_block_t {
	packed_spc_shared = 1,
	packed_spc_unique,
	packed_spc_0x00,
	packed_spc_0xFF
};

#ifdef __cplusplus
	}
#endif

#endif

