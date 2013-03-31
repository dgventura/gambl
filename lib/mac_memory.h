
// Mac memory functions that throw exceptions and do debug checking of parameters

// Game_Music_Box 0.5.2. Copyright (C) 2005 Shay Green. GNU LGPL license.

#ifndef MAC_MEMORY_H
#define MAC_MEMORY_H

#include "common.h"

void* NewPtrChk( long );
long GetPtrSizeChk( void* );
void SetPtrSizeChk( void*, long );
void DisposePtrChk( void* );

Handle NewHandleChk( long );
Handle TempNewHandleNothrow( long );
Handle TempNewHandleChk( long );
Handle RecoverHandleChk( void* );
// to do: consider changing to void* instead of handle to avoid user casts
long GetHandleSizeChk( Handle );
void SetHandleSizeChk( Handle, long );
void DisposeHandleChk( Handle );

#endif

